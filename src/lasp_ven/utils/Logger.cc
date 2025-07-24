///*
// * Logger.cc
// *
// * Utility class for logging simulation events and performance metrics
// */
//
//#include <fstream>
//#include <iostream>
//#include <iomanip>
//#include <sstream>
//#include <ctime>
//
//namespace lasp_ven {
//
//class Logger {
//private:
//    static std::ofstream logFile;
//    static bool initialized;
//    static std::string logLevel;
//
//public:
//    enum Level {
//        DEBUG = 0,
//        INFO = 1,
//        WARN = 2,
//        ERROR = 3
//    };
//
//    static void initialize(const std::string& filename, const std::string& level = "INFO") {
//        if (!initialized) {
//            logFile.open(filename, std::ios::app);
//            logLevel = level;
//            initialized = true;
//
//            // Write header
//            logFile << "\n" << std::string(80, '=') << "\n";
//            logFile << "LASP VEN Simulation Log - " << getCurrentTimestamp() << "\n";
//            logFile << std::string(80, '=') << "\n";
//        }
//    }
//
//    static void logServiceRequest(int vehicleId, int serviceType, double timestamp,
//                                 double lat, double lon, int priority) {
//        if (!shouldLog(INFO)) return;
//
//        logFile << "[" << std::fixed << std::setprecision(3) << timestamp << "] "
//                << "SERVICE_REQUEST | Vehicle: " << vehicleId
//                << " | Service: " << serviceType
//                << " | Position: (" << std::setprecision(6) << lat << ", " << lon << ")"
//                << " | Priority: " << priority << "\n";
//        logFile.flush();
//    }
//
//    static void logServicePlacement(int serviceId, int serverId, int serviceType,
//                                   double latency, double timestamp, const std::string& strategy) {
//        if (!shouldLog(INFO)) return;
//
//        logFile << "[" << std::fixed << std::setprecision(3) << timestamp << "] "
//                << "SERVICE_PLACEMENT | Service: " << serviceId
//                << " | Server: " << serverId
//                << " | Type: " << serviceType
//                << " | Latency: " << std::setprecision(2) << latency << "ms"
//                << " | Strategy: " << strategy << "\n";
//        logFile.flush();
//    }
//
//    static void logServerStatus(int serverId, double load, double capacity,
//                               double utilization, double timestamp) {
//        if (!shouldLog(DEBUG)) return;
//
//        logFile << "[" << std::fixed << std::setprecision(3) << timestamp << "] "
//                << "SERVER_STATUS | Server: " << serverId
//                << " | Load: " << std::setprecision(2) << load
//                << " | Capacity: " << capacity
//                << " | Utilization: " << std::setprecision(1) << utilization * 100 << "%\n";
//        logFile.flush();
//    }
//
//    static void logStrategyPerformance(const std::string& strategy, int totalRequests,
//                                     int servedRequests, double avgLatency,
//                                     double avgUtilization, double timestamp) {
//        if (!shouldLog(INFO)) return;
//
//        logFile << "[" << std::fixed << std::setprecision(3) << timestamp << "] "
//                << "STRATEGY_PERFORMANCE | Strategy: " << strategy
//                << " | Requests: " << totalRequests
//                << " | Served: " << servedRequests
//                << " | Success Rate: " << std::setprecision(1)
//                << (double)servedRequests/totalRequests * 100 << "%"
//                << " | Avg Latency: " << std::setprecision(2) << avgLatency << "ms"
//                << " | Avg Utilization: " << std::setprecision(1) << avgUtilization * 100 << "%\n";
//        logFile.flush();
//    }
//
//    static void logError(const std::string& message, double timestamp) {
//        if (!shouldLog(ERROR)) return;
//
//        logFile << "[" << std::fixed << std::setprecision(3) << timestamp << "] "
//                << "ERROR | " << message << "\n";
//        logFile.flush();
//    }
//
//    static void logWarning(const std::string& message, double timestamp) {
//        if (!shouldLog(WARN)) return;
//
//        logFile << "[" << std::fixed << std::setprecision(3) << timestamp << "] "
//                << "WARNING | " << message << "\n";
//        logFile.flush();
//    }
//
//    static void logDebug(const std::string& message, double timestamp) {
//        if (!shouldLog(DEBUG)) return;
//
//        logFile << "[" << std::fixed << std::setprecision(3) << timestamp << "] "
//                << "DEBUG | " << message << "\n";
//        logFile.flush();
//    }
//
//    static void writeStatistics(const std::map<std::string, double>& statistics) {
//        if (!initialized) return;
//
//        logFile << "\n" << std::string(50, '-') << "\n";
//        logFile << "SIMULATION STATISTICS\n";
//        logFile << std::string(50, '-') << "\n";
//
//        for (std::map<std::string, double>::const_iterator it = statistics.begin();
//             it != statistics.end(); ++it) {
//            logFile << std::left << std::setw(30) << it->first << ": "
//                    << std::right << std::setw(10) << std::fixed << std::setprecision(3)
//                    << it->second << "\n";
//        }
//        logFile << std::string(50, '-') << "\n";
//        logFile.flush();
//    }
//
//    static void close() {
//        if (initialized && logFile.is_open()) {
//            logFile << "\nLog closed at " << getCurrentTimestamp() << "\n";
//            logFile.close();
//            initialized = false;
//        }
//    }
//
//private:
//    static bool shouldLog(Level level) {
//        if (!initialized) return false;
//
//        Level currentLevel = INFO;
//        if (logLevel == "DEBUG") currentLevel = DEBUG;
//        else if (logLevel == "INFO") currentLevel = INFO;
//        else if (logLevel == "WARN") currentLevel = WARN;
//        else if (logLevel == "ERROR") currentLevel = ERROR;
//
//        return level >= currentLevel;
//    }
//
//    static std::string getCurrentTimestamp() {
//        auto now = std::time(nullptr);
//        auto* ltm = std::localtime(&now);
//        std::stringstream ss;
//        ss << std::put_time(ltm, "%Y-%m-%d %H:%M:%S");
//        return ss.str();
//    }
//};
//
//// Static member definitions
//std::ofstream Logger::logFile;
//bool Logger::initialized = false;
//std::string Logger::logLevel = "INFO";
//
//} // namespace lasp_ven
