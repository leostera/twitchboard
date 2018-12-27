let version = "0.1";

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

module Server = {
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
};

let login = _ => {
  Sys.(set_signal(sigpipe, Signal_ignore));

  switch (Bos.OS.Dir.create(Config.default_config_path)) {
  | Ok(_) =>
    Logs.app(m => m("Beginning login flow..."));
    let run_server =
      Server.serve(~port=Config.default_port, ~on_start=_server =>
        Logs_lwt.debug(m =>
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

    switch (Rresult.(Secrets.read_default() >>| Secrets.token)) {
    | exception err =>
      Logs.err(m => m("Somethiing went south %s", Printexc.to_string(err)))
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

  | Error(`Msg(msg)) => Logs.err(m => m("Something went wrong: %s", msg))
  };
};
