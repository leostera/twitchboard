/**
	* Originally from: https://github.com/anmonteiro/aws-lambda-ocaml-runtime/blob/master/examples/now-lambda-reason/basic.re
	*/
module Response = {
  let read_body = (response, body) => {
    let buffer = Buffer.create(1024);
    let (next, wakeup) = Lwt.wait();
    switch (Httpaf.Response.(response.status)) {
    | `OK =>
      let rec read_response = () =>
        Httpaf.Body.schedule_read(
          body,
          ~on_eof=
            () => Lwt.wakeup_later(wakeup, Ok(Buffer.contents(buffer))),
          ~on_read=
            (response_fragment, ~off, ~len) => {
              let response_fragment_string = Bytes.create(len);
              Lwt_bytes.blit_to_bytes(
                response_fragment,
                off,
                response_fragment_string,
                0,
                len,
              );
              Buffer.add_bytes(buffer, response_fragment_string);
              read_response();
            },
        );
      read_response();
    | _ =>
      Logs.err(m => m("Something went wrong"));
      Lwt.wakeup_later(wakeup, Error(`Reading_error));
    };
    next;
  };
};

let writev = (tls_client, _fd, io_vecs) =>
  Lwt.(
    catch(
      () => {
        let {Faraday.len, buffer, off} = io_vecs |> List.hd;
        Cstruct.of_bigarray(~len, ~off, buffer)
        |> Tls_lwt.Unix.write(tls_client)
        >|= (_ => `Ok(len));
      },
      exn =>
        switch (exn) {
        | Unix.Unix_error(Unix.EBADF, "check_descriptor", _) =>
          Lwt.return(`Closed)
        | exn =>
          Logs_lwt.err(m => m("failed: %s", Printexc.to_string(exn)))
          >>= (() => Lwt.fail(exn))
        },
    )
  );

let read = (tls_client, _fd, buffer) =>
  Lwt.(
    catch(
      () =>
        buffer
        |> Httpaf_lwt.Buffer.put(~f=(bigstring, ~off, ~len) =>
             Tls_lwt.Unix.read_bytes(tls_client, bigstring, off, len)
           ),
      exn =>
        Logs_lwt.err(m => m("Https.read: %s", Printexc.to_string(exn)))
        >>= (
          () => {
            Lwt.async(() => Tls_lwt.Unix.close(tls_client));
            Lwt.fail(exn);
          }
        ),
    )
    >|= (
      bytes_read =>
        if (bytes_read == 0) {
          `Eof;
        } else {
          `Ok(bytes_read);
        }
    )
  );

let send = (~meth=`GET, ~headers=[], ~body=?, uri) => {
  open Httpaf;
  open Httpaf_lwt;
  open Lwt.Infix;
  let response_handler = (notify_response_received, response, response_body) =>
    Lwt.wakeup_later(
      notify_response_received,
      Ok((response, response_body)),
    );

  let error_handler = (notify_response_received, error) =>
    Lwt.wakeup_later(notify_response_received, Error(error));

  let host = Uri.host_with_default(uri);

  Lwt_unix.getaddrinfo(host, "443", [Unix.(AI_FAMILY(PF_INET))])
  >>= (
    addresses => {
      let socket_addr = List.hd(addresses).Unix.ai_addr;
      let socket = Lwt_unix.socket(Unix.PF_INET, Unix.SOCK_STREAM, 0);

      X509_lwt.authenticator(`No_authentication_I'M_STUPID)
      >>= (
        authenticator => {
          let client = Tls.Config.client(~authenticator, ());
          Lwt_unix.connect(socket, socket_addr)
          >>= (_ => Tls_lwt.Unix.client_of_fd(client, ~host, socket));
        }
      )
      >>= (
        tls_client => {
          let content_length =
            switch (body) {
            | None => "0"
            | Some(body) => string_of_int(String.length(body))
            };

          let request_headers =
            Request.create(
              meth,
              Uri.path_and_query(uri),
              ~headers=
                Headers.of_list(
                  [("Host", host), ("Content-Length", content_length)]
                  @ headers,
                ),
            );

          let (response_received, notify_response_received) = Lwt.wait();
          let response_handler = response_handler(notify_response_received);
          let error_handler = error_handler(notify_response_received);

          let request_body =
            Client.request(
              ~writev=writev(tls_client),
              ~read=read(tls_client),
              socket,
              request_headers,
              ~error_handler,
              ~response_handler,
            );

          switch (body) {
          | Some(body) => Body.write_string(request_body, body)
          | None => ()
          };
          Body.flush(request_body, () => Body.close_writer(request_body));
          response_received;
        }
      );
    }
  );
};
