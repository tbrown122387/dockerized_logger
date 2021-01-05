# Interactive Brokers Gateway in Docker 

## Overview

The terms of service on data feeds usually stipulate that you cannot share the data with anyone, so this repo is to help students/anyone download their own data from an Interactive Brokers account. It can also be used as a stepping stone for writing a dockerized execution client. 

This repo modifies the work [here](https://github.com/dvasdekis/ib-gateway-docker-gcp).

## Quick start guide

On a host machine that you have shell access to (remote or local), clone this repo, change the config files, then type 

```docker-compose up``` 

or 

```docker-compose up -d``` 

if you want to run everything in the background. Every time this starts up, it will trigger a 2FA popup on your cell phone. 

Depending on how you installed Docker and Docker-Compose, you might need to add a `sudo` in front of those commands above. The first time you do this it will take a while--that is because it is building the entire Docker image. The good news is that the second time around everything is much quicker.


If you want that to run every day automatically on a Linux host, you can make it a `cron` job. This is what I do. I'm sure there are analog tools for other OSs. 

To do that, type `sudo crontab -e`, and then add the following lines: 

```
# start up gateway through ibc every weekday at 8:30AM EST 
30 8 * * 1-5 cd ~/dockerized_logger && /usr/local/bin/docker-compose up

# nightly reboot at 4:05AM
0 4   *   *   *    /sbin/shutdown -r +5
```


### Config Files

These are what you need to change before you `docker-compose up` everything. The config files are the ones with sensitive information, so be sure not to publicly share your edited version of this code. Or if you do, exclude these files. 

They are
 
1. your Interactive Brokers user id and password are going to be stored in `.env` 

2. your mysql database information is stored in `dockerized_logger/log_app/ib_client/IBJts/samples/Cpp/TestCppClient/mysql_config.txt`

3. the symbols you are interested in tracking (at the moment this is futures only!) are in `dockerized_logger/log_app/ib_client/IBJts/samples/Cpp/TestCppClient/tickers.txt`

### Tips

The following mistakes don't really show up in the logs, so be careful:

1. using the wrong username/password, 

2. using the account or paper/live setting your data subscription is not associated with

3. specifying paper/live and forgetting to change `IB_GATEWAY_URLPORT` in `docker-compose.yml`.


