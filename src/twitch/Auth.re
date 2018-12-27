module Token = {
  type t = string;

  let of_string = str => str;

  let to_string = token => token;

  let pp = x => x;
};

module ResponseType = {
  type t = [ | `Token];
  let to_string =
    fun
    | `Token => "token";
};

module Scope = {
  type t = [ | `Channel_read];

  let to_string = scope =>
    switch (scope) {
    | `Channel_read => "channel_read"
    };

  let to_scopelist = scopes =>
    scopes |> List.map(to_string) |> String.concat(",");
};

let build_login_url = (~scopes, ~redirect_uri, ~response_type, client_id) =>
  "https://id.twitch.tv/oauth2/authorize?client_id="
  ++ client_id
  ++ "&redirect_uri="
  ++ redirect_uri
  ++ "&response_type="
  ++ ResponseType.to_string(response_type)
  ++ "&scope="
  ++ Scope.to_scopelist(scopes);
