open Notty

let p (_ow, _oh) (_w, _h) = I.string (let open A in (fg red) ++ (bg black)) "Wow!"
