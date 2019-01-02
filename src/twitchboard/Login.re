module LoginFlow = {
  let start = () => {
    Logs.app(m => m("Beginning login flow..."));

    let run_server =
      AuthServer.serve(~port=Config.default_port, ~on_start=_server =>
        Logs.debug(m =>
          m(
            "Running auth server at https://localhost:%d",
            Config.default_port,
          )
        )
      );

    let auth_url =
      Config.default_client_id
      |> Twitch.Auth.build_login_url(
           ~scopes=[`Channel_read],
           ~redirect_uri=Config.default_redirect_uri,
           ~response_type=`Token,
         );

    Logs.debug(m => m("Opening: %s", auth_url));

    let res = Sys.command("open \"" ++ auth_url ++ "\"");
    switch (res) {
    | 0 => ()
    | _ => Logs.err(m => m("Something went wrong!"))
    };

    run_server |> Lwt_main.run;

    Logs.app(m => m("Success!"));
  };
};

module Greeting = {
  let print = () =>
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
              Logs_lwt.app(m =>
                m(
                  {j|Welcome, %s
Your configuration is saved in %s
You may start using twitchboard now :)|j},
                  user.display_name,
                  Config.default_config_path |> Fpath.to_string,
                )
              )
            | Error(`Unauthorized(_)) =>
              Logs_lwt.err(m => m("Unauthorized request to Twitch API."))
            | Error(`Reading_error) =>
              Logs_lwt.err(m => m("while reading the error response."))
            | Error(`Connection_error(_)) =>
              Logs_lwt.err(m => m("while connecting to the Twitch API."))
            | Error(`Response_error(_)) =>
              Logs_lwt.err(m => m("while requesting from the Twitch API."))
            }
        )
        |> Lwt_main.run
      );
    };
};

let login = _ => {
  Sys.(set_signal(sigpipe, Signal_ignore));

  switch (Config.ensure()) {
  | Error(_) => Logs.err(m => m("Oops! Something went wrong 🙈"))
  | Ok(`Existing_secrets) => Greeting.print()
  | Ok(`Created)
  | Ok(`No_secrets) =>
    LoginFlow.start();
    Greeting.print();
  };
};
