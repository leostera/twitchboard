let greet = () =>
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

let login_please = () =>
  Logs.err(m => m("Please make sure you have ran: twitchboard login"));

module Errors = {
  let to_user_message = err =>
    switch (err) {
    | `Unauthorized(_) => "Unauthorized request to Twitch API."
    | `Reading_error => "while reading the error response."
    | `Connection_error(_) => "while connecting to the Twitch API."
    | `Response_error(_) => "while requesting from the Twitch API."
    };
};
