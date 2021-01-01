#ifndef TICK_WRITER_H
#define TICK_WRITER_H

#include <string>
#include <map>
#include <cppconn/driver.h> 
#include <chrono>

#include "config.h"


//* TODOs (maybe put a separate class and in a separate header)
// * 3. u-pnl for each instrument
// * 4. r-pnl for each instrument
// * 5. open orders 
// * 6. open positions 


/* hft namespace  */
namespace hft {


/* type alias for clock type */
using ClockType = std::chrono::high_resolution_clock;

/* type alias for time point  */
using TimePoint = std::chrono::time_point<ClockType>;


/* convert chrono time points to strings for writing out to mysql */
inline std::string toString(const TimePoint& time);


/**
 * @struct UserPassCredentials
 * @brief stores a username and password
 */
struct UserPassCredentials {

    std::string username;
    std::string password;
};


/**
 * @struct MySqlConfig
 * @brief stores database info
 */
struct MySqlConfig {

    /* the database */
    std::string database;

    /* the table for orders */
    std::string orderTable;

    /* the table for orders */
    std::string tradeTable;

    /* the host */
    std::string host;

    /* the port */
    int port;

    /* the username and password */
    UserPassCredentials credentials;

    /**
     * @brief reads the config from the specified file, with the following format
     *
     * -------------------
     * database=dbName
     * orderTable=tableName
     * tradeTable=tableName
     * host=HostName
     * port=PortNumber
     * user=Username
     * password=Password
     * -------------------
     *
     *
     * @param path the file path
     * @return parsed config
     */
    static MySqlConfig readConfigFromFile(const std::string& path);


};


/**
 * @struct BidAsk
 * @brief top of order book at one moment
 */
struct BidAsk {
    TimePoint tp;
    double bidPrice;
    double askPrice;
    int bidSize;
    int askSize;
    std::string instrument;
};


/**
 * @struct Trade
 * @brief an executed trade 
 */
struct Trade {
    TimePoint tp;
    double price;
    int size;
    std::string exchange;
    std::string instrument;
};


/**
 * @class TickWriter
 * @brief instantiate once as a trade client member
 * and logs the following information on each price 
 * update (TODO: size updates as welll)
 *
 * 1. bid for instrument 
 * 2. ask for instrument 
 * 
 */
class TickWriter : public FutSymsConfig {

public:

    /**
     * @brief constructor instantiates driver and opens connection
     * @param msql_config
     * @param sym_table_file (same as the one provides to trade client)
     * @param printing
     * @param reconnect
     * @param autoFlushEvery 0 means never
     */
    explicit TickWriter(const std::string& mysql_cnfg_file, 
                        const std::string& sym_table_file, 
                        unsigned autoFlushEvery = 0,
                        bool printing = false,
                        bool reconnect = true);


    /**
     * @brief destroys pointers to avoid memory leaks
     */
    ~TickWriter(); 


    /**
     * @brief adds bid/ask to its bundle 
     */
    void addBidAsk(const TimePoint& dt, 
                   double bidPrice, 
                   double askPrice,
                   int bidSize,
                   int askSize,    
                   const std::string& instrument); 


    /**
     * @brief adds trade to its bundle 
     */
    void addTrade(const TimePoint& dt, 
                   double price, 
                   int size,
                   const std::string& exchange,
                   const std::string& instrument); 


    /**
     * @brief writes all the elements in the list to a database
     */
    void flushToDB();


    /**
     * @brief returns the number of orders that have not
     * yet been written to a database
     */
    unsigned sizeOrders() const;


    /**
     * @brief returns the number of trades that have not
     * yet been written to a database
     */
    unsigned sizeTrades() const;
 
private:
 
   
    /* the database configuration */
    MySqlConfig m_msql_config;

    /* the driver */
    sql::Driver* m_driver;

    /* the connection pointer */
    sql::Connection* m_conn;

    /* whether or not to print when you add rows */
    bool m_printing;

    /* an arbitrarily-long collection of order info */ 
    std::list<BidAsk> m_order_bundle; 

    /* an arbitrarily-long collection of trade info */ 
    std::list<Trade> m_trade_bundle; 

    /* running total of the number of data points seen  */
    unsigned m_num_data;

    /* how often do you want to flush data to db? */
    unsigned m_auto_flush_every;
};

} // namespace hft
#endif // TICK_WRITER_H
