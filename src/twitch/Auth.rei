module Token: {
  /** The type of a token */
  type t;
  /** Creates a token from a string */
  let of_string: string => t;
  let to_string: t => string;
  let pp: t => string;
};

module ResponseType: {
  /** The type of the response types */
  type t = [ | `Token];
  /** Get the string representation of a response type */
  let to_string: t => string;
};

module Scope: {
  type t = [ | `Channel_read];
  let to_string: t => string;
  let to_scopelist: list(t) => string;
};

let build_login_url:
  (
    ~scopes: list(Scope.t),
    ~redirect_uri: string,
    ~response_type: ResponseType.t,
    string
  ) =>
  string;
