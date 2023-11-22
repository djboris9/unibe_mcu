# Zephyr Project setup
Following https://docs.zephyrproject.org/3.5.0/develop/application/index.html#application for a freestanding application.

```bash
~/zephyr-sdk-0.16.1/sysroots/x86_64-pokysdk-linux/usr/bin/openocd -s ~/zephyr-sdk-0.16.1/sysroots/x86_64-pokysdk-linux/usr/share/openocd/scripts/ -f x/openocd.cfg -c init -c targets -c 'reset init'
```

`openocd.cfg` is from `/home/vscode/zephyrproject/zephyr/boards/arm/nucleo_f446re/support/openocd.cfg`.