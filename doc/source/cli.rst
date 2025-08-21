PyECLib CLI
===========

PyECLib provides a ``pyeclib-backend`` tool to provide various information about backends.

``version`` subcommand
----------------------
.. code:: text

   pyeclib-backend [-V | version]

Displays the versions of pyeclib, liberasurecode, and python.

``list`` subcommand
-------------------
.. code:: text

   pyeclib-backend list [-a | --available] [<ec_type>]

Displays the status (available, missing, or unknwon) of requested backends.
By default, all backends are displayed; if ``--available`` is provided, only
available backends are displayed, and status is not displayed.

``check`` subcommand
--------------------
.. code:: text

   pyeclib-backend check [-q | --quiet] <ec_type>

Check whether a backend is available. Exits

- 0 if ``ec_type`` is available,
- 1 if ``ec_type`` is missing, or
- 2 if ``ec_type`` is not recognized

If ``--quiet`` is provided, output nothing; rely only on exit codes.

``verify`` subcommand
---------------------
.. code:: text

   pyeclib-backend verify [-q | --quiet] [--ec-type=all]
       [--n-data=10] [--n-parity=5] [--unavailable=2] [--segment-size=1024]

Verify the ability to decode all combinations of fragments given some number
of unavailable fragments.

``bench`` subcommand
--------------------
.. code:: text

   pyeclib-backend bench [-e | --encode] [-d | --decode] [--ec-type=all]
       [--n-data=10] [--n-parity=5] [--unavailable=2] [--segment-size=1048576]
       [--iterations=200]

Benchmark one or more backends.
