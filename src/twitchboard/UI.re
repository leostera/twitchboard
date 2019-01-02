open Lwt.Infix;
open Notty;
module Term = Notty_lwt.Term;

let terminal = (~imgf, ~f, ~s) => {
  let term = Term.create();
  let imgf = ((w, h), s) =>
    I.(string(A.(fg(lightblack)), "[ESC quits.]") <-> imgf((w, h - 1), s));
  let step = (e, s) =>
    switch (e) {
    | `Key(`Escape, [])
    | `Key(`ASCII('C'), [`Ctrl]) => Term.release(term) >|= (() => s)
    | `Resize(dim) => Term.image(term, imgf(dim, s)) >|= (() => s)
    | #Unescape.event as e =>
      switch (f(s, e)) {
      | Some(s) => Term.image(term, imgf(Term.size(term), s)) >|= (() => s)
      | None => Term.release(term) >|= (() => s)
      }
    };

  Term.image(term, imgf(Term.size(term), s))
  >>= (() => Lwt_stream.fold_s(step, Term.events(term), s));
};
