#include "tick_writer.h"

#include <fstream> //ifstream
#include <sstream> //stringstream
#include <memory> // unique_ptr
#include <cppconn/prepared_statement.h> // preparedStatement
#include <boost/algorithm/string.hpp>


namespace hft{


inline std::string toString(const TimePoint& time) {
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch());
    std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(ms);
    std::time_t t = s.count();
    std::size_t fractional_seconds = ms.count() %1000;
    char cstr[100];
    std::strftime(cstr, sizeof(cstr), "%Y-%m-%d %H:%M:%OS", std::localtime(&t));
    return std::string(cstr) + std::string(".") + std::to_string(fractional_seconds);
}


    
MySqlConfig MySqlConfig::readConfigFromFile(const std::string &path) {

    // make properties
    std::map<std::string, std::string> properties;
    std::ifstream file(path);
    if (file.good()) {
        std::string line;
        while (getline(file, line)) {
            std::stringstream stream(line);
            std::string name;
            std::string elem;
            getline(stream, name, '=');
            getline(stream, elem);
            boost::algorithm::trim(name);
            boost::algorithm::trim(elem);
            properties[name] = elem;
        }
       
        file.close();

    }else{
        std::cerr << "\nmysql file not good\n";
    }

    UserPassCredentials credentials {properties.at("user"), properties.at("password")};
    return MySqlConfig {
        properties.at("database"), 
        properties.at("orderTable"),
        properties.at("tradeTable"),
        properties.at("host"), 
        std::stoi(properties.at("port")), 
        credentials
    };
}

TickWriter::TickWriter(const std::string& mysql_cnfg_file,
                       const std::string& sym_table_file, 
                       unsigned autoFlushEvery,
                       bool printing,
                       bool reconnect)
    : FutSymsConfig(sym_table_file)
    , m_printing(printing)
    , m_num_data(0)
    , m_auto_flush_every(autoFlushEvery)
{ 

    // read in mysql config file
    m_msql_config = MySqlConfig::readConfigFromFile(mysql_cnfg_file); 

    // database stuff
    // configure driver and connection
    m_driver = get_driver_instance();
    std::string conn_str = "tcp://" 
                         + m_msql_config.host + ":" 
                         + std::to_string(m_msql_config.port);
    m_conn = m_driver->connect(conn_str, 
                               m_msql_config.credentials.username, 
                               m_msql_config.credentials.password);
    m_conn->setClientOption("OPT_RECONNECT", &reconnect); 
    m_conn->setSchema(m_msql_config.database);

}
       

TickWriter::~TickWriter()
{
    delete m_conn;
}


void TickWriter::addBidAsk(
        const TimePoint& dt,  
        double bidPrice, 
        double askPrice, 
        int bidSize, 
        int askSize,
        const std::string& instrument)
{
    m_order_bundle.push_back(BidAsk{dt, bidPrice, askPrice, bidSize, askSize, instrument });
    m_num_data++;
    if( m_auto_flush_every > 0){
        if( m_num_data % m_auto_flush_every == 0)
            flushToDB(); 
    }
}


void TickWriter::addTrade(
        const TimePoint& dt,  
        double price, 
        int size, 
        const std::string& exchange,
        const std::string& instrument)
{
    m_trade_bundle.push_back(Trade{dt, price, size, exchange, instrument });    
    m_num_data++;
    if( m_auto_flush_every > 0){
        if( m_num_data % m_auto_flush_every == 0)
            flushToDB(); 
    }
}


void TickWriter::flushToDB()
{
    // print to see
    if(m_printing)
        std::cout << "attempting to write data for symbols\n";

    // step 1: write order information
    try{
        for(const auto& tick : m_order_bundle) {
        
            // make the symbol uppercase just in case (pun intended)
            std::string sql = "INSERT INTO " 
                            + m_msql_config.database + "." + m_msql_config.orderTable  
                            + " VALUES ('"
                            + hft::toString(tick.tp) + "', "
                            + std::to_string(tick.bidPrice) + ", "
                            + std::to_string(tick.askPrice) + ", "
                            + std::to_string(tick.bidSize) + ", "
                            + std::to_string(tick.askSize) + ", '"
                            + tick.instrument + "');";
            if(m_printing)
                std::cout << sql << "\n";        
            std::unique_ptr<sql::Statement> p_stmnt(m_conn->createStatement());
            p_stmnt->execute(sql);
        }
    
    }catch(const std::exception& e){
        std::cerr << "flushToDB problem: " << e.what() << "\n"; 
    }catch(...){
        std::cerr << "unspecified flushToDB problem\n";
    }
    m_order_bundle.clear();  


    // step 2: write trade information
    try{
        for(const auto& trade : m_trade_bundle) {
        
            // make the symbol uppercase just in case (pun intended)
            std::string sql = "INSERT INTO " 
                            + m_msql_config.database + "." + m_msql_config.tradeTable  
                            + " VALUES ('"
                            + hft::toString(trade.tp) + "', "
                            + std::to_string(trade.price) + ", "
                            + std::to_string(trade.size) + ", '" 
                            + trade.exchange+ "', '"
                            + trade.instrument + "');";

            if(m_printing)
                std::cout << sql << "\n";        
            std::unique_ptr<sql::Statement> p_stmnt(m_conn->createStatement());
            p_stmnt->execute(sql);
        }
    
    }catch(const std::exception& e){
        std::cerr << "flushToDB problem: " << e.what() << "\n"; 
    }catch(...){
        std::cerr << "unspecified flushToDB problem\n";
    }
    m_trade_bundle.clear();   


}

unsigned TickWriter::sizeOrders() const
{
    return m_order_bundle.size();
}


unsigned TickWriter::sizeTrades() const
{
    return m_trade_bundle.size();
}

} // namespace hft
