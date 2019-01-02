module Stream = {
  type t = {
    id: string,
    viewer_count: int,
    title: string,
    started_at: Ptime.t,
  };

  let from_json = json => {
    open Yojson.Basic.Util;
    let started_at =
      json |> member("started_at") |> to_string |> Ptime.of_rfc3339;

    switch (started_at) {
    | Ok((started_at, _, _)) =>
      let viewer_count = json |> member("viewer_count") |> to_int;
      let id = json |> member("id") |> to_string;
      let title = json |> member("title") |> to_string;
      Ok({viewer_count, id, title, started_at});
    | Error(_) =>
      switch (Ptime.rfc3339_error_to_msg(started_at)) {
      | Ok(_) => Error(`How_did_we_get_here)
      | Error(`Msg(msg)) => Error(`Parse_error(msg))
      }
    };
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
          /* | `OK => Httpkit.Client.Response.body(res) */
          | `OK =>
            Lwt_result.lift(
              Ok(
                {j|
{
  "data": [
    {
      "id": "31974264048",
      "user_id": "169615083",
      "user_name": "ostera",
      "game_id": "509670",
      "community_ids": [],
      "type": "live",
      "title": "Reasonable Coding",
      "viewer_count": 0,
      "started_at": "2019-01-02T18:18:15Z",
      "language": "en",
      "thumbnail_url": "https://static-cdn.jtvnw.net/previews-ttv/live_user_ostera-{width}x{height}.jpg",
      "tag_ids": [
        "6f86127d-6051-4a38-94bb-f7b475dde109",
        "c23ce252-cf78-4b98-8c11-8769801aaf3a",
        "dff0aca6-52fe-4cc4-a93a-194852b522f0",
        "7b49f69a-5d95-4c94-b7e3-66e2c0c6f6c6",
        "a59f1e4e-257b-4bd0-90c7-189c3efbf917",
        "6ea6bca4-4712-4ab9-a906-e3336a9d8039"
      ]
    }
  ],
  "pagination": {
    "cursor": "eyJiIjpudWxsLCJhIjp7Ik9mZnNldCI6MX19"
  }
}
|j},
              ),
            )
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
