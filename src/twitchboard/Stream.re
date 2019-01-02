module LiveStream = {
  let print_stream: Twitch.Streams.Stream.t => Lwt.t(unit) =
    stream =>
      Logs_lwt.app(m =>
        m("Stream ID: %s\tViewer Count: %d\n", stream.id, stream.viewer_count)
      );

  let stats = () =>
    switch (Rresult.(Secrets.read_default() >>| Secrets.token)) {
    | exception err =>
      Logs.err(m => m("Something went south %s", Printexc.to_string(err)))
    | Error(`File_error(msg)) => Logs.err(m => m("File error %s", msg))
    | Error(`Parse_error(msg)) => Logs.err(m => m("Parse error %s", msg))
    | Error(`Invalid_secrets) => Logs.err(m => m("Invalid secrets"))
    | Ok(token) =>
      Logs.debug(m => m("Read Token: %s", Twitch.Auth.Token.pp(token)));
      Lwt.Infix.(
        token
        |> Twitch.Users.get_token_bearer
        >>= (
          result =>
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
        |> Lwt_main.run
      );
    };
};

let stream = () =>
  switch (Config.ensure()) {
  | Error(_) => Logs.err(m => m("Oops! Something went wrong 🙈"))
  | Ok(`Existing_secrets) => LiveStream.stats()
  | _ => Messages.login_please()
  };
