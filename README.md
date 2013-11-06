james-squid-redirector
======================

an Squid Url redirect plugin/filter for static images cache
At start read libconfig file /etc/james.conf to read sql and paths information  
then read caching domain from database and rewrite urls from stdin to stdout

### example: ###

> http://www.inmyclub.net/image?url=n9zj5dWjkprmppyprZmbo5zTndjcmMzY2piXYqGkmKCjzZ7Zw93Emu%2BmYWhmZm9mp9re28vVyJnjoZk%3D

> http://127.0.0.1/cache/inmyclub.net/image%3furl=n9zj5dWjkprmppyprZmbo5zTndjcmMzY2piXYqGkmKCjzZ7Zw93Emu%2BmYWhmZm9mp9re28vVyJnjoZk%3D


### libconfig file contents example: ###

> dbhost    = "localhost";

> dbname    = "dashboard";

> dblogin   = "username";

> dbpassw   = "password";

> interface = "eth0";

> cachepath = "/cache/inmyclub.net/"; 

> webroot   = "/var/www/";


__use binary in squid.conf as url_rewrite_program argument__
