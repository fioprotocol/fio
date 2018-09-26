#include "client_http.hpp"
#include "server_http.hpp"

// Added for the json-example
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

// Added for the default_resource example
#include <algorithm>
#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#ifdef HAVE_OPENSSL
#include "crypto.hpp"
#endif

const  std::string dapixaddress = "http://localhost:8889";
const  std::string walletpassword = "PW5HtY2mVLhpL3ohFmQoqj7mwFTNc9shVeP91x3gXKthKmLbubaL5"; //wallet password, has private key for exchange1111

using namespace std;
// Added for the json-example:
using namespace boost::property_tree;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

int main(int argc, char *argv[]) {
	

  // HTTP-server at port 8080 using 1 thread
  // Unless you do more heavy non-threaded processing in the resources,
  // 1 thread is usually faster than several threads
  HttpServer server;
  server.config.port = 8080;

  server.resource["^/fio/registername$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    
	 string success = "command failure"; 
	 try {
      ptree pt;
      read_json(request->content, pt);

      auto name = pt.get<string>("name") ;
		

	  //Unlock the wallet
	  stringstream fioaction1;
	  fioaction1<<"cleos -u "<<dapixaddress<<" wallet unlock --password \""<<walletpassword<<"\"";
	  system(fioaction1.str().c_str());
	  
	  //Issue the action
	  stringstream fioaction2;
	  fioaction2<<"cleos -u "<<dapixaddress<<" push action -j exchange1111 registername \'{\"name\":\""<<name<<"\"}\' --permission exchange1111@active";
	  if(!system(fioaction2.str().c_str()))
	  {
			cout<<fioaction2.str().c_str();
		  	success = "\ncommand executed successfully";
	  }

		 
	 //console output, std stream
	 cout<<"\nregistername was executed by remote request." << endl;
		

      *response << "HTTP/1.1 200 OK\r\n"
                << "Content-Length: " << success.length() << "\r\n\r\n"
                << success;
    }
    catch(const exception &e) {
      *response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n"
                << e.what();
    }

	  
  };

  server.resource["^/fio/addaddress$"]["POST"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
    
	 string success = "command failure"; 
	 try {
      ptree pt;
      read_json(request->content, pt);

      auto fio_name = pt.get<string>("fio_name") ;
	  auto address = pt.get<string>("address") ;
	  auto chain = pt.get<string>("chain") ;
		

	  //Unlock the wallet
	  stringstream fioaction1;
	  fioaction1<<"cleos -u "<<dapixaddress<<" wallet unlock --password \""<<walletpassword<<"\"";
	  system(fioaction1.str().c_str());
	  
	  //Issue the action
	  stringstream fioaction2;
	  fioaction2<<"cleos -u "<<dapixaddress<<" push action -j exchange1111 addaddress \'{\"fio_name\":\""<<fio_name<<"\",\"address\":\""<<address<<"\",\"chain\":\""<<chain<<"\"}\' --permission exchange1111@active";
	  if(!system(fioaction2.str().c_str()))
	  {
			cout<<fioaction2.str().c_str();
		  	success = "\ncommand executed successfully";
	  }

		 
	 //console output, std stream
	 cout<<"\naddaddress was executed by remote request." << endl;
		

      *response << "HTTP/1.1 200 OK\r\n"
                << "Content-Length: " << success.length() << "\r\n\r\n"
                << success;
    }
    catch(const exception &e) {
      *response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n"
                << e.what();
    }

	  
  };

	
	
	
	
	
	

  server.on_error = [](shared_ptr<HttpServer::Request> /*request*/, const SimpleWeb::error_code & /*ec*/) {
    // Handle errors here
    // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
  };

  thread server_thread([&server]() {
    // Start server
    server.start();

	  
  });
	
	
	
	
/* 
  // Wait for server to start so that the client can connect
  this_thread::sleep_for(chrono::seconds(1));

  Client examples
    HttpClient client("localhost:8080");


  // Synchronous request examples
  try {

    auto r1 = client.request("POST", "/string", json_string);
    cout << r1->content.rdbuf() << endl;
  }
  catch(const SimpleWeb::system_error &e) {
    cerr << "Client request error: " << e.what() << endl;
  }

  // Asynchronous request example
  client.request("POST", "/json", json_string, [](shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code &ec) {
    if(!ec)
      cout << response->content.rdbuf() << endl;
  });
 
  client.io_service->run();
	*/
	
		  
cout<<endl<<endl<<"FIO API IS CURRENTLY RUNNING...."<<endl;
cout<<endl<<endl<<"PORT: "<<server.config.port<<endl;
	
  server_thread.join();

	
	return EXIT_SUCCESS;
	
}
