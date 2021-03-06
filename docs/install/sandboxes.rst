.. _install_sandboxes:

Sandboxes
=========

The docker-compose sandboxes give you different environments to test out Envoy's
features. As we gauge people's interests we will add more sandboxes demonstrating
different features.

Front Proxy
-----------

To get a flavor of what Envoy has to offer as a front proxy, we are releasing a
`docker compose <https://docs.docker.com/compose/>`_ sandbox that deploys a front
envoy and a couple of services (simple flask apps) colocated with a running
service envoy. The three containers will be deployed inside a virtual network
called ``envoymesh``.

Below you can see a graphic showing the docker compose deployment:

.. image:: /_static/docker_compose_v0.1.svg
  :width: 100%

All incoming requests are routed via the front envoy, which is acting as a reverse proxy sitting on
the edge of the ``envoymesh`` network. Port ``80`` is mapped to  port ``8000`` by docker compose
(see :repo:`/examples/front-proxy/docker-compose.yml`). Moreover, notice
that all  traffic routed by the front envoy to the service containers is actually routed to the
service envoys (routes setup in :repo:`/examples/front-proxy/front-envoy.json`). In turn the service
envoys route the  request to the flask app via the loopback address (routes setup in
:repo:`/examples/front-proxy/service-envoy.json`). This setup
illustrates the advantage of running service envoys  collocated with your services: all requests are
handled by the service envoy, and efficiently routed to your services.

Running the Sandbox
~~~~~~~~~~~~~~~~~~~

The following documentation runs through the setup of an envoy cluster organized
as is described in the image above.

**Step 1: Install Docker**

Ensure that you have a recent versions of ``docker, docker-compose`` and
``docker-machine`` installed.

A simple way to achieve this is via the `Docker Toolbox <https://www.docker.com/products/docker-toolbox>`_.

**Step 2: Docker Machine setup**

First let's create a new machine which will hold the containers::

    $ docker-machine create --driver virtualbox default
    $ eval $(docker-machine env default)

**Step 4: Clone the Envoy repo, and start all of our containers**

If you have not cloned the envoy repo, clone it with ``git clone git@github.com:lyft/envoy``
or ``git clone https://github.com/lyft/envoy.git``::

    $ pwd
    envoy/example
    $ docker-compose up --build -d
    $ docker-compose ps
            Name                       Command               State      Ports
    -------------------------------------------------------------------------------------------------------------
    example_service1_1      /bin/sh -c /usr/local/bin/ ...    Up       80/tcp
    example_service2_1      /bin/sh -c /usr/local/bin/ ...    Up       80/tcp
    example_front-envoy_1   /bin/sh -c /usr/local/bin/ ...    Up       0.0.0.0:8000->80/tcp, 0.0.0.0:8001->8001/tcp

**Step 5: Test Envoy's routing capabilities**

You can now send a request to both services via the front-envoy.

For service1::

    $ curl -v $(docker-machine ip default):8000/service/1
    *   Trying 192.168.99.100...
    * Connected to 192.168.99.100 (192.168.99.100) port 8000 (#0)
    > GET /service/1 HTTP/1.1
    > Host: 192.168.99.100:8000
    > User-Agent: curl/7.43.0
    > Accept: */*
    >
    < HTTP/1.1 200 OK
    < content-type: text/html; charset=utf-8
    < content-length: 89
    < x-envoy-upstream-service-time: 1
    < server: envoy
    < date: Fri, 26 Aug 2016 19:39:19 GMT
    < x-envoy-protocol-version: HTTP/1.1
    <
    Hello from behind Envoy (service 1)! hostname: f26027f1ce28 resolvedhostname: 172.19.0.6
    * Connection #0 to host 192.168.99.100 left intact

For service2::

    $ curl -v $(docker-machine ip default):8000/service/2
    *   Trying 192.168.99.100...
    * Connected to 192.168.99.100 (192.168.99.100) port 8000 (#0)
    > GET /service/2 HTTP/1.1
    > Host: 192.168.99.100:8000
    > User-Agent: curl/7.43.0
    > Accept: */*
    >
    < HTTP/1.1 200 OK
    < content-type: text/html; charset=utf-8
    < content-length: 89
    < x-envoy-upstream-service-time: 2
    < server: envoy
    < date: Fri, 26 Aug 2016 19:39:23 GMT
    < x-envoy-protocol-version: HTTP/1.1
    <
    Hello from behind Envoy (service 2)! hostname: 92f4a3737bbc resolvedhostname: 172.19.0.2
    * Connection #0 to host 192.168.99.100 left intact

Notice that each request, while sent to the front envoy, was correctly routed
to the respective application.

**Step 6: Test Envoy's load balancing capabilities**

Now let's scale up our service1 nodes to demonstrate the clustering abilities
of envoy.::

    $ docker-compose scale service1=3
    Creating and starting example_service1_2 ... done
    Creating and starting example_service1_3 ... done

Now if we send a request to service1 multiple times, the front envoy will load balance the
requests by doing a round robin of the three service1 machines::

    $ curl -v $(docker-machine ip default):8000/service/1
    *   Trying 192.168.99.100...
    * Connected to 192.168.99.100 (192.168.99.100) port 8000 (#0)
    > GET /service/1 HTTP/1.1
    > Host: 192.168.99.100:8000
    > User-Agent: curl/7.43.0
    > Accept: */*
    >
    < HTTP/1.1 200 OK
    < content-type: text/html; charset=utf-8
    < content-length: 89
    < x-envoy-upstream-service-time: 1
    < server: envoy
    < date: Fri, 26 Aug 2016 19:40:21 GMT
    < x-envoy-protocol-version: HTTP/1.1
    <
    Hello from behind Envoy (service 1)! hostname: 85ac151715c6 resolvedhostname: 172.19.0.3
    * Connection #0 to host 192.168.99.100 left intact
    $ curl -v $(docker-machine ip default):8000/service/1
    *   Trying 192.168.99.100...
    * Connected to 192.168.99.100 (192.168.99.100) port 8000 (#0)
    > GET /service/1 HTTP/1.1
    > Host: 192.168.99.100:8000
    > User-Agent: curl/7.43.0
    > Accept: */*
    >
    < HTTP/1.1 200 OK
    < content-type: text/html; charset=utf-8
    < content-length: 89
    < x-envoy-upstream-service-time: 1
    < server: envoy
    < date: Fri, 26 Aug 2016 19:40:22 GMT
    < x-envoy-protocol-version: HTTP/1.1
    <
    Hello from behind Envoy (service 1)! hostname: 20da22cfc955 resolvedhostname: 172.19.0.5
    * Connection #0 to host 192.168.99.100 left intact
    $ curl -v $(docker-machine ip default):8000/service/1
    *   Trying 192.168.99.100...
    * Connected to 192.168.99.100 (192.168.99.100) port 8000 (#0)
    > GET /service/1 HTTP/1.1
    > Host: 192.168.99.100:8000
    > User-Agent: curl/7.43.0
    > Accept: */*
    >
    < HTTP/1.1 200 OK
    < content-type: text/html; charset=utf-8
    < content-length: 89
    < x-envoy-upstream-service-time: 1
    < server: envoy
    < date: Fri, 26 Aug 2016 19:40:24 GMT
    < x-envoy-protocol-version: HTTP/1.1
    <
    Hello from behind Envoy (service 1)! hostname: f26027f1ce28 resolvedhostname: 172.19.0.6
    * Connection #0 to host 192.168.99.100 left intact

**Step 7: enter containers and curl services**

In addition of using ``curl`` from your host machine, you can also enter the
containers themselves and ``curl`` from inside them. To enter a container you
can use ``docker-compose exec <container_name> /bin/bash``. For example we can
enter the ``front-envoy`` container, and ``curl`` for services locally::

  $ docker-compose exec front-envoy /bin/bash
  root@81288499f9d7:/# curl localhost:80/service/1
  Hello from behind Envoy (service 1)! hostname: 85ac151715c6 resolvedhostname: 172.19.0.3
  root@81288499f9d7:/# curl localhost:80/service/1
  Hello from behind Envoy (service 1)! hostname: 20da22cfc955 resolvedhostname: 172.19.0.5
  root@81288499f9d7:/# curl localhost:80/service/1
  Hello from behind Envoy (service 1)! hostname: f26027f1ce28 resolvedhostname: 172.19.0.6
  root@81288499f9d7:/# curl localhost:80/service/2
  Hello from behind Envoy (service 2)! hostname: 92f4a3737bbc resolvedhostname: 172.19.0.2

**Step 8: enter containers and curl admin**

When envoy runs it also attaches an ``admin`` to your desired port. In the example
configs the admin is bound to port ``8001``. We can ``curl`` it to gain useful information.
For example you can ``curl`` ``/server_info`` to get information about the
envoy version you are running. Addionally you can ``curl`` ``/stats`` to get
statistics. For example inside ``frontenvoy`` we can get::

  $ docker-compose exec front-envoy /bin/bash
  root@e654c2c83277:/# curl localhost:8001/server_info
  envoy 10e00b/RELEASE live 142 142 0
  root@e654c2c83277:/# curl localhost:8001/stats
  cluster.service1.external.upstream_rq_200: 7
  ...
  cluster.service1.membership_change: 2
  cluster.service1.membership_total: 3
  ...
  cluster.service1.upstream_cx_http2_total: 3
  ...
  cluster.service1.upstream_rq_total: 7
  ...
  cluster.service2.external.upstream_rq_200: 2
  ...
  cluster.service2.membership_change: 1
  cluster.service2.membership_total: 1
  ...
  cluster.service2.upstream_cx_http2_total: 1
  ...
  cluster.service2.upstream_rq_total: 2
  ...

Notice that we can get the number of members of upstream clusters, number of requests
fulfilled by them, information about http ingress, and a plethora of other useful
stats.

gRPC bridge
-----------

Envoy gRPC
~~~~~~~~~~

The gRPC bridge sandbox is an example usage of Envoy's
:ref:`gRPC bridge filter <config_http_filters_grpc_bridge>`.
Included in the sandbox is a gRPC in memory Key/Value store with a Python HTTP
client. The Python client makes HTTP/1 requests through the Envoy sidecar
process which are upgraded into HTTP/2 gRPC requests. Response trailers are then
buffered and sent back to the client as a HTTP/1 header payload.

Another Envoy feature demonstrated in this example is Envoy's ability to do authority
base routing via its route configuration.

Building the Go service
~~~~~~~~~~~~~~~~~~~~~~~

To build the Go gRPC service run::

  $ pwd
  ~/src/envoy/examples/grpc-bridge
  $ script/bootstrap
  $ script/build

Docker compose
~~~~~~~~~~~~~~

To run the docker compose file, and set up both the Python and the gRPC containers
run::

  $ pwd
  ~/src/envoy/examples/grpc-bridge
  $ docker-compose up --build

Sending requests to the Key/Value store
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To use the python service and sent gRPC requests::
  $ pwd
  ~/src/envoy/examples/grpc-bridge
  # set a key
  $ docker-compose exec python /client/client.py set foo bar
  setf foo to bar

  # get a key
  $ docker-compose exec python /client/client.py get foo
  bar

Locally building a docker image with an envoy binary
----------------------------------------------------

The following steps guide you through building your own envoy binary, and
putting that in a clean ubuntu container.

**Step 1: Build Envoy**

Using ``lyft/envoy-build`` you will compile envoy.
This image has all software needed to build envoy. From your envoy directory::

  $ pwd
  src/envoy
  $ docker run -t -i -v <SOURCE_DIR>:/source lyft/envoy-build:latest /bin/bash -c "cd /source && ci/do_ci.sh normal"

That command will take some time to run because it is compiling an envoy binary.

**Step 2: Build image with only envoy binary**

In this step we'll build an image that only has the envoy binary, and none
of the software used to build it.::

  $ pwd
  src/envoy/
  $ docker build -f ci/Dockerfile-envoy-image -t envoy .

Now you can use this ``envoy`` image to build the any of the sandboxes if you change
the ``FROM`` line in any dockerfile.

This will be particularly useful if you are interested in modifying envoy, and testing
your changes.
