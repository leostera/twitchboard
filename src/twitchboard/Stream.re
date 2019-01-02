open Lwt.Infix;

module LiveStream = {
  let print_stream: Twitch.Streams.Stream.t => Lwt.t(unit) =
    _stream =>
      UI.terminal(~s=(80, 40), ~f=(a, _) => Some(a), ~imgf=UI_Stream.p)
      >|= (_ => ());

  let stats = () =>
    switch (Rresult.(Secrets.read_default() >>| Secrets.token)) {
    | exception err =>
      Logs.err(m => m("Something went south %s", Printexc.to_string(err)))
    | Error(`File_error(msg)) => Logs.err(m => m("File error %s", msg))
    | Error(`Parse_error(msg)) => Logs.err(m => m("Parse error %s", msg))
    | Error(`Invalid_secrets) => Logs.err(m => m("Invalid secrets"))
    | Ok(token) =>
      Logs.debug(m => m("Read Token: %s", Twitch.Auth.Token.pp(token)));
      let (loop, _finish) = Lwt.wait();
      let rec work = result =>
        Lwt.Infix.(
          (
            switch (result) {
            | Ok(user) =>
              Twitch.Streams.stats(token, user, ~count=1)
              >>= (
                results =>
                  switch (results) {
                  | Ok([]) =>
                    Logs_lwt.app(m => m("No active live-streams found!"))
                  | Ok(streams) => streams |> Lwt_list.iter_s(print_stream)
                  | Error(err) =>
                    Logs_lwt.err(m =>
                      m("%s", err |> Messages.Errors.to_user_message)
                    )
                  }
              )
            | Error(err) =>
              Logs_lwt.err(m =>
                m("%s", err |> Messages.Errors.to_user_message)
              )
            }
          )
          >>= (_ => Lwt_unix.sleep(1.0))
          >>= (_ => work(result))
        );

      Lwt.Infix.(
        Lwt.async(() =>
          token
          |> Twitch.Users.get_token_bearer
          >>= work
          >>= (_ => Lwt.return_unit)
        )
      );

      loop |> Lwt_main.run;
    };
};

let stream = () =>
  switch (Config.ensure()) {
  | Error(_) => Logs.err(m => m("Oops! Something went wrong 🙈"))
  | Ok(`Existing_secrets) => LiveStream.stats()
  | _ => Messages.login_please()
  };
