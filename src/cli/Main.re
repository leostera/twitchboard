open Cmdliner;

let setup_verbosity = (level, debug) => {
  Fmt_tty.setup_std_outputs();
  if (debug) {
    Logs.set_level(Some(Logs.Debug));
  } else {
    Logs.set_level(level);
  };
  Logs.set_reporter(Logs_fmt.reporter());
};

module Args = {
  let debug = {
    let doc = "Shortcut for debugging verbosity";
    Arg.(value & flag & info(["d", "debug"], ~doc));
  };

  let verbosity = Term.(const(setup_verbosity) $ Logs_cli.level() $ debug);
};

module Login = {
  let cmd = {
    let doc = "login to Twitch.tv";
    let exits = Term.default_exits;
    let man = [
      `S(Manpage.s_description),
      `P(
        {j|Initialize login flow against Twitch.tv and save credentials in disk.|j},
      ),
    ];

    (
      Term.(const(Twitchboard.Login.login) $ Args.verbosity),
      Term.info("login", ~doc, ~sdocs=Manpage.s_common_options, ~exits, ~man),
    );
  };
};

module Stream = {
  let cmd = {
    let doc = "get stream statistics";
    let exits = Term.default_exits;
    let man = [
      `S(Manpage.s_description),
      `P(
        {j|Display real-time statistics about your currently open stream.|j},
      ),
      `P({j|Exits immediately if you don't have any active streams.|j}),
    ];

    (
      Term.(const(Twitchboard.Stream.stream) $ Args.verbosity),
      Term.info(
        "stream",
        ~doc,
        ~sdocs=Manpage.s_common_options,
        ~exits,
        ~man,
      ),
    );
  };
};

let default_cmd = {
  let doc = "Real-time stream stats for Twitch.tv";
  let sdocs = Manpage.s_common_options;
  let exits = Term.default_exits;
  (
    Term.(ret(const(`Help((`Pager, None))))),
    Term.info(
      "twitchboard",
      ~version=Twitchboard.version,
      ~doc,
      ~sdocs,
      ~exits,
    ),
  );
};

Term.([Stream.cmd, Login.cmd] |> eval_choice(default_cmd) |> exit);
