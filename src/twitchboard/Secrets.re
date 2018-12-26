type t = {token: string};

let of_token: string => t = token => {token: token};

let token = t => t.token |> Twitch.Auth.Token.of_string;

let write: (Fpath.t, t) => result(unit, _) =
  (path, secret) => {
    let value = Sexplib0.Sexp.List([Atom("token"), Atom(secret.token)]);
    value |> Sexplib0.Sexp.to_string |> Bos.OS.File.write(path);
  };

let read = path =>
  Rresult.R.(
    path
    |> Bos.OS.File.read
    |> reword_error((`Msg(err)) => `File_error(err))
    >>= (
      file =>
        file
        |> Parsexp.Many.parse_string
        |> reword_error(err =>
             `Parse_error(Parsexp.Parse_error.message(err))
           )
    )
    >>= (
      values =>
        switch (values) {
        | [Sexplib0.Sexp.List([Atom("token"), Atom(token)])] =>
          Ok(of_token(token))
        | _ => Error(`Invalid_secrets)
        }
    )
  );

let read_default = () => read(Config.default_secret_path);
