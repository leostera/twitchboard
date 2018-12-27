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

    Ok({login, display_name, id});
  };
};

let get_token_bearer: Auth.Token.t => Lwt.t(result(User.t, _)) =
  token =>
    Lwt_result.Infix.(
      "https://api.twitch.tv/helix/users"
      |> Uri.of_string
      |> Httpkit.Client.Https.send(
           ~headers=[
             ("Authorization", "Bearer " ++ (token |> Auth.Token.to_string)),
           ],
         )
      >>= (
        ((response, _) as res) =>
          switch (response.status) {
          | `OK => Httpkit.Client.Response.body(res)
          | `Unauthorized => Lwt_result.fail(`Unauthorized(res))
          | _ => Lwt_result.fail(`Response_error(response.status))
          }
      )
      >>= (
        body =>
          body
          |> Yojson.Basic.from_string
          |> Yojson.Basic.Util.member("data")
          |> Yojson.Basic.Util.index(0)
          |> User.from_json
          |> Lwt_result.lift
      )
    );
