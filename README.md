snodfish
========

SnodFish is an open source API back-end server.
Rather than run something like _haproxy_ in front of _nginx_, and then your choice of front-end
(such as _sinatra_ for Ruby heads), SnodFish listens on a well-known port,
accepts REST commands over an HTTP interface,
and translates them into JSON RPC calls to one or more back-end servers.

You create a mapping by defining how you want the GET, PUT, POST and DELETE urls to be handled,
and the URL as well as any passed-in JSON is executed by one or more back-ends via one or more
RPC mechanisms.

For now, the only supported RPC is ZeroMQ.
