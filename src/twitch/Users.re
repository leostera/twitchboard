module User = {
  type t = {
    login: string,
    display_name: string,
    id: string,
  };

  let from_json = json => {
    open Yojson.Basic.Util;
    let login = json |> member("login") |> to_string;
    let display_name = json |> member("display_name") |> to_string;
    let id = json |> member("id") |> to_string;

    Logs.debug(m => m("User.from_json"));

    Ok({login, display_name, id});
  };
};

let get_token_bearer: Auth.Token.t => Lwt.t(result(User.t, _)) =
  token =>
    Lwt.Infix.(
      "https://api.twitch.tv/helix/users"
      |> Uri.of_string
      |> Httpkit.Client.Https.send(~headers=[token |> Auth.Token.as_header])
      >>= Httpkit.Client.Response.body
      >|= (body => Rresult.(body >>= User.from_json))
    );
