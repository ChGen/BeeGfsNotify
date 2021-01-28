This library allows to track beeGFS v7 distributed filesystem (DFS) changes and delivers it by ZeroMQ.
  When tracking DFS changes one may periodically scan entire storage, which is highly inefficient. Another option is too utilize specific DFS features. BeeGFS since version 7 supports logging file system changes in binary form to local UNIX socket for external use.
  BeeGfsNotify library connects beeGFS output with filtering features and ZeroMQ-backed delivery system, which allows to track beeGFS storage efficiently and conveniently on any DFS client in the network.

# Usage

1. Example:

	```cpp

	#include "DfsNotifier.h"

	auto fsNotifier = std::make_shared<DfsNotifier>(context , socketPath);
	std::thread notifierThread(&DfsNotifier::run, fsNotifier);
	//...
	std::string subId = fsNotifier->checkPathWithSubId(dfsPath);
	//...
	subId = fsNotifier->rmPathWithSubId(dfsPath);
	//...
	fsNotifier->stop()
	```

2. Public API:

	```cpp
	/**
	 * @brief The DfsNotifier class allows to subscribe on DFS path changes and filter them before deliver.
	 * DFS changes delivered through ZeroMQ messaging technology.
	 */
	class DfsNotifier {
	//...
	    /**
	     * @brief run - start reading and processing events (synchronous)
	     */
	    void run();

	    /**
	     * @brief stop reading and processing events. Waits until processing finishes.
	     */
	    void stop();

	    /**
	     * @brief check path registration and return subsquent subscriber Id
	     * @param path is full DFS path to watch
	     * @return - subscriber id, matching given path
	     */
	    std::string checkPathWithSubId(const std::string path);

	    /**
	     * @brief rmPathWithSubId
	     * @param path
	     * @return
	     */
	    std::string rmPathWithSubId(const std::string &path);
	//...
	};

	```


# Install

1. Install CMake 3.16 or newer, GCC 7.0 or newer.

2. Add BeeGfsNotify project as git submodule (the best way):
	```
	git submodule add git@github.com:ChGen/BeeGfsNotify.git sub/BeeGfsNotify
	```
2. Check test project `sub/BeeGfsNotify/CMakeLists.txt`:

	```
	cd sub/BeeGfsNotify
	mkdir build && cd build
	conan install ..
	cmake ..
	make -rj4
	./beegfs_notify_test/beegfs_notify_test
	```

3. Add to your project's `CMakeLists.txt`
	```cpp
	add_subdirectory(sub/BeeGfsNotify/beegfs_notify_server_lib)
	target_link_libraries(app beegfs_notify_lib)
	```

# Contributing

Contributions are definitely welcomed. Please, consider providing GitHub ticket first with full disclosure. When providing MR, follow code style and ensure all tests are passed.

