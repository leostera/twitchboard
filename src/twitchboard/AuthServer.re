module Endpoints = {
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

    let headers = [("Content-Length", "0")];
    closer();
    `With_headers((`OK, headers, ""));
  };

  let auth_callback = () => {
    Logs.info(m => m("Login flow completed."));
    let content_len = website |> String.length |> string_of_int;
    let headers = [("Content-Length", content_len)];
    `With_headers((`OK, headers, website));
  };
};

let route_handler: Httpkit.Server.Common.route_handler(unit) =
  (ctx, path) =>
    switch (path) {
    | ["auth", "success", token] =>
      Endpoints.auth_success(~closer=ctx.closer, token)
    | ["auth", "callback"] => Endpoints.auth_callback()
    | _ => `Unmatched
    };

let serve = (~port, ~on_start) =>
  Httpkit.(
    Server.Infix.(
      Server.make()
      *> Server.Common.log
      << Server.Common.router(route_handler)
      |> Http.listen(~port, ~on_start)
    )
  );
