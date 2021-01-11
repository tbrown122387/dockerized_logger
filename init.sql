CREATE DATABASE IF NOT EXISTS ib;

CREATE TABLE IF NOT EXISTS ib.bid_ask_data (
dt datetime(6) NOT NULL,
bidPrice DECIMAL(12, 5) NOT NULL,
askPrice DECIMAL(12, 5) NOT NULL,
bidSize INT(12) NOT NULL,
askSize INT(12) NOT NULL,
instrument VARCHAR(15) NOT NULL,
PRIMARY KEY (dt, instrument, bidPrice, askPrice, bidSize, askSize)
);

CREATE TABLE IF NOT EXISTS ib.trade_data (
dt datetime(6) NOT NULL,
price DECIMAL(12, 5) NOT NULL,
size INT(12) NOT NULL,
exchange VARCHAR(15) NOT NULL,
instrument VARCHAR(15) NOT NULL,
PRIMARY KEY (dt, instrument, price, size, exchange)
);

