# 📺 Tw!tchBoard — Real-time Stream Stats Tool for Twitch.tv

As part of one of the episodes of [Reasonable Coding](https://twitch.tv/ostera) 
we started building this command-line tool for Twitch.tv to get stream
statistics in real-time right in your terminal.

[Watch the stream here](https://www.twitch.tv/videos/354544842) — you're
warned: it's really long (~7 hours), due to debugging a TLS error in our http
library.

**Motivation**. Why on earth? Glad you asked. I mostly live on my terminal, so
switching to my phone for the Twitch app or alt-tabbing to a browser in a
different desktop is very disruptive while streaming.

Having a small window with the stats, however, is quite okay 👌🏼.

Thus began `twitchboard`. 

The scope of it can be seen in the
[SCOPE.md](https://github.com/ostera/twitchboard/tree/master/SCOPE.md)
document. But we covered quite a few things:

* Built a nice CLI for it
* Read and Save config files to disk
* Local HTTP Server serving a static page from memory and handling authentication flows
* Requests to the Twitch API
* Modeling the Twitch API into Reason libraries
* JSON Parsing
* Tons of Async stuff with Lwt going on!
* Tons of Low-level HTTPS/TLS things going on too!

In fact, I've extracted the HTTPS code (which was originally from @anmonteiro) 
and I'll be experimenting with higher-level APIs for building type-safe native 
HTTP/S servers and clients here: [ostera/httpkit](https://github.com/ostera/httpkit).

## Getting Started

> Note: this tool has not been published yet! So you can't install it with `brew`
> or your favorite package manager. Maybe sometime in the future :)

As of now you can install with `opam`:

```sh
$ opam pin add twitchboard git+https://github.com/ostera/twitchboard
```

Or if you'd rather install from source manually:

```sh
$ git clone https://github.com/ostera/twitchboard
$ cd twitchboard
$ dune build @install
$ dune install
```

## Using Twitchboard

The only flow available right now is the `login` flow:

```sh
ostera/twitchboard λ twitchboard login
Beginning login flow...
Welcome, ostera
Your configuration is saved in ~/.twitchboard/
You may start using twitchboard now :)
```

This will automatically open up a browser window to do the authentication, and
will figure the rest of the thing out for you 🙌🏼— no annoying link copying or
anything. Just a seamless flow.
