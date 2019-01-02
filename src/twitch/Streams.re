module Stream = {
  type t = {
    id: string,
    viewer_count: int,
  };

  let from_json = json => {
    open Yojson.Basic.Util;
    let viewer_count = json |> member("viewer_count") |> to_int;
    let id = json |> member("id") |> to_string;

    Ok({viewer_count, id});
  };

  let list_from_json = jsonlist =>
    Rresult.R.(
      List.fold_left(
        (acc, json) =>
          switch (acc) {
          | Ok(last) =>
            switch (json |> from_json) {
            | Ok(this) => Ok([this, ...last])
            | Error(_) as err => err
            }
          | Error(_) as err => err
          },
        Ok([]),
        jsonlist,
      )
      >>| List.rev
    );
};

let stats:
  (Auth.Token.t, ~count: int, Users.User.t) =>
  Lwt.t(result(list(Stream.t), _)) =
  (token, ~count, user) =>
    Lwt_result.Infix.(
      "https://api.twitch.tv/helix/streams?first="
      ++ (count |> string_of_int)
      ++ "&user_id="
      ++ user.id
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
          |> Yojson.Basic.Util.to_list
          |> Stream.list_from_json
          |> Lwt_result.lift
      )
    );
