#include <cstdlib>
#include <iostream>
#include <string>
#include <experimental/filesystem>
#include <memory>
#include <string>
#include <stdexcept>
#include <thread>
#include <zmqpp/zmqpp.hpp>
#include "Common.h"
#include "DfsEventsWatcher.h"

int main (int argc, char *argv[]) {

    if (argc != 2) {
        std::cerr << "Usage ./client_test <path>" << std::endl;
        return 1;
    }

    zmqpp::context ctx;

    DfsEventsWatcher watcher(ctx);
    watcher.start("tcp://localhost:" + std::to_string(BeegfsEvents::NOTIFICATION_SERVICE_PORT),
                  [](const std::string& watchedDir, const std::string& changedObject, const std::string& operation) {
        std::cout << "Callback: watchedDir: " << watchedDir << " ;changedObject: " << changedObject <<
                     " ;operation: " << operation << std::endl;
    });
    bool result = watcher.addPath(argv[1]);
    if (!result) {
        std::cerr << "Failed to watch on path!" << std::endl;
        return 2;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1000L));

    watcher.stop();
    std::cout << "Done." << std::endl;
    return 0;
}
