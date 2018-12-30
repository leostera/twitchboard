module Endpoints = {
  open Httpaf;
  let website = {j|
  <script>
    let access_token = window.location.hash //"#a=1&b=2"
      .substr(1) // "a=1&b=2"
      .split("&") // [ "a=1", "b=2"]
      .map(x => x.split("=")) // [ ["a", "1"], ["b", "2"]]
      .reduce( (acc, kv) => ({...acc, [kv[0]]: kv[1] }), {}) // { a:"1", b:"2"}
      .access_token

    fetch("/auth/success/"+access_token)
      .then( res => {
        self.close();
      });
  </script>
|j};

  let auth_success = (~closer, token) => {
    Logs.info(m => m("Received token %s", token));

    let res =
      token |> Secrets.of_token |> Secrets.write(Config.default_secret_path);

    switch (res) {
    | Ok(_) => ()
    | Error(`Msg(msg)) => Logs.err(m => m("Something went wrong! %s", msg))
    };

    let headers = Headers.of_list([("Content-Length", "0")]);
    Lwt.wakeup_later(closer, ());
    (Response.create(`OK, ~headers), "");
  };

  let auth_callback = () => {
    Logs.info(m => m("Login flow completed."));
    let content_len = website |> String.length |> string_of_int;
    let headers = Headers.of_list([("Content-Length", content_len)]);
    (Response.create(`OK, ~headers), website);
  };
};

let request_handler = (~closer, _client, req_desc) => {
  open Httpaf;
  let req = Reqd.request(req_desc);
  Logs.info(m => m("Auth Server %s", req.target));
  let path = req.target |> String.split_on_char('/') |> List.tl;
  let (res, contents) =
    switch (path) {
    | ["auth", "success", token] => Endpoints.auth_success(~closer, token)
    | ["auth", "callback"] => Endpoints.auth_callback()
    | _ =>
      Logs.info(m => m("Unknown path %s", req.target));
      let headers = Headers.of_list([("Content-Length", "0")]);
      (Response.create(`Method_not_allowed, ~headers), "");
    };
  Reqd.respond_with_string(req_desc, res, contents);
};

let error_handler = (client, ~request=?, error, fn) => {
  client |> ignore;
  request |> ignore;
  error |> ignore;
  fn |> ignore;
};

let connection_handler = (~closer) =>
  Httpaf_lwt.Server.create_connection_handler(
    ~request_handler=request_handler(~closer),
    ~error_handler,
  );

let serve = (~port, ~on_start) => {
  open Lwt.Infix;
  let (forever, close) = Lwt.wait();

  let listening_address = Unix.(ADDR_INET(inet_addr_loopback, port));

  Lwt_io.establish_server_with_client_socket(
    listening_address,
    connection_handler(~closer=close),
  )
  >>= on_start
  |> ignore;

  forever;
};
