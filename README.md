# bcp-examples
the client buillds for android use kivy , compiles using buildozer 



Make targets:
- venv: sets up the venv (python 3.14)


# Stocks 
```

Edit the service file:
```
sudo vim /lib/systemd/system/bluetooth.service
```
Modify the ExecStart line:
```
# Find: ExecStart=/usr/lib/bluetooth/bluetoothd
# hange to: ExecStart=/usr/lib/bluetooth/bluetoothd -C  (The -C stands for Compatibility)
```


Reload and Restart:
```
sudo systemctl daemon-reload
sudo systemctl restart bluetooth
```

