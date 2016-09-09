# Installing MariaDB MaxScale using a tarball

MariaDB MaxScale is also made available as a tarball, which is named like `maxscale-X.Y.X.tar.gz` where `X.Y.Z` is the same as the corresponding version, e.g. `maxscale-2.0.1.tar.gz`.

The tarball has been built with the assumption that it will be installed in `/usr/local`. However, it is possible to install it in any directory, but in that case MariaDB MaxScale must be invoked with a flag.

## Installing as root in /usr/local

If you have root access to the system you probably want to install MariaDB MaxScale under the user and group `maxscale`.

The required steps are as follows:

    $ sudo groupadd maxscale
    $ sudo useradd -g maxscale maxscale
    $ cd /usr/local
    $ sudo tar -xzvf maxscale-X.Y.Z.tar.gz
    $ sudo ln -s maxscale-X.Y.Z maxscale
    $ cd maxscale
    $ chown -R maxscale var

Creating the symbolic link is necessary, since MariaDB MaxScale has been built with with the assumption that its base-directory is is `/usr/local/maxscale`. It also makes it easy to switch between different versions of MariaDB MaxScale that have been installed side by side in `/usr/local`.

The following step is to create the MariaDB MaxScale configuration file `/etc/maxscale.cnf`. The file `etc/maxscale.cnf.template` can be used as a base. Please refer to [Configuration Guide](Configuration-Guide.md) for details.

When the configuration file has been created, MariaDB MaxScale can be started.

    $ sudo su -u maxscale bin/maxscale -d

The `-d` flag causes maxscale _not_ to turn itself into a daemon, which is adviseable to do the first time MariaDB MaxScale is started, as it makes it easier to spot problems.

If you want to place the configuration file somewhere else but in `/etc` you can either invoke MariaDB MaxScale, for instance, with --config=/usr/local/maxscale/etc/maxscale.cnf`.

# Installing Anywhere

Enter a directory where you have the right to create a subdirectory.
Then do as follows.

    $ tar -xzvf maxscale-X.Y.Z.tar.gz

The following step is to create the MaxScale configuration file `maxscale-X.Y.Z/etc/maxscale.cnf`. The file `maxscale-X.Y.Z/etc/maxscale.cnf.template` can be used as a base. Please refer to [Configuration Guide](Configuration-Guide.md) for details.

When the configuration file has been created, MariaDB MaxScale can be started.

    $ cd maxscale-X.Y.Z
    $ LD_LIBRARY_PATH=lib/maxscale bin/maxscale --basedir=.

With the flag `--basedir`, MariaDB MaxScale is told where the `bin`, `etc`, `lib`
and `var` directories are found. Unless it is specified, MariaDB MaxScale assumes
the directories are found in `/usr/local/maxscale` and the configuration
file in `/etc`.

It is also possible to specify the directories and the location of the
configuration file individually. Invoke MaxScale as

    $ LD_LIBRARY_PATH=lib/maxscale bin/maxscale --help

to find out the appropriate flags.
