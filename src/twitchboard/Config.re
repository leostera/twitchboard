let default_port = 50505;
let default_client_id = "br6f9ygrqa2llnyctgh048vgd5i0pd";
let default_redirect_uri =
  "http://localhost:" ++ string_of_int(default_port) ++ "/auth/callback";

let home = Sys.getenv("HOME") |> Fpath.v;
let default_config_path = Fpath.(home / ".twitchboard") |> Fpath.to_dir_path;
let default_secret_path = Fpath.(default_config_path / "secrets");
