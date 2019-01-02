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

let login = _ => {
  Sys.(set_signal(sigpipe, Signal_ignore));

  switch (Config.ensure()) {
  | Error(_) => Logs.err(m => m("Oops! Something went wrong 🙈"))
  | Ok(`Existing_secrets) => Messages.greet()
  | Ok(`Created)
  | Ok(`No_secrets) =>
    LoginFlow.start();
    Messages.greet();
  };
};
