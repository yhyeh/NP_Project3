#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <string>
#include <sstream>
#include <vector>


using boost::asio::ip::tcp;

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(rdata_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            /* view request */
            std::string req(rdata_);
            std::cout << req << std::endl;
            /* parse request */
            std::istringstream issReq(req);
            std::string lineInReq;
            for (int iLine = 0; iLine < 2; iLine++){
              std::getline(issReq, lineInReq);
              if (iLine == 0){
                /* GET /panel.cgi HTTP/1.1 */
                /* GET /console.cgi?h0=nplinux1.cs.nctu.edu.tw&p0= ... */
                std::istringstream issLine(lineInReq);
                std::string wordInLine;
                for(int iWord = 0; iWord < 3; iWord++){
                  std::getline(issLine, wordInLine, ' ');
                  if(iWord == 0){
                    /* GET */
                    setenv("REQUEST_METHOD", wordInLine.c_str(), 1);
                  }
                  else if (iWord == 1){
                    /* /panel.cgi */
                    /* /console.cgi?h0=nplinux1.cs.nctu.edu.tw&p0= ... */
                    std::string file = wordInLine.substr(1, wordInLine.find_first_of(".")-1);
                    if (file == "panel"){
                      isPanel = true;
                      setenv("REQUEST_URI", wordInLine.c_str(), 1);
                    }else if(file == "console"){
                      isConsole = true;
                      file = "/" + file + ".cgi";
                      setenv("REQUEST_URI", file.c_str(), 1);
                      std::string query = wordInLine.substr(wordInLine.find_first_of("?")+1);
                      setenv("QUERY_STRING", query.c_str(), 1);
                      //std::cout << getenv("QUERY_STRING") << std::endl;
                    }
                  }
                  else{
                    /* HTTP/1.1 */
                    setenv("SERVER_PROTOCOL", wordInLine.c_str(), 1);
                  }
                }
              }
              else if (iLine == 1){
                /* Host: localhost:18787 */
                setenv("HTTP_HOST", lineInReq.substr(6).c_str(), 1);  // remove "Host: "
              }             
            }
            /* set REMOTE_ADDR, REMOTE_PORT */
            setenv("REMOTE_ADDR", socket_.remote_endpoint().address().to_string().c_str(), 1);
            setenv("REMOTE_PORT", std::to_string(socket_.remote_endpoint().port()).c_str(), 1);
            setenv("DOCUMENT_ROOT", ".", 1);

            /* panel.cgi or printenv.cgi*/
            if (!isConsole){
              int pid;
              if ((pid = fork()) == -1){
                std::cerr << "fork: " << strerror(errno) << std::endl;
                exit(0);
              }
              if (pid == 0){ //child
                dup2(socket_.native_handle(), STDOUT_FILENO);
                for (int fd = 3; fd <= __FD_SETSIZE; fd++){
                  close(fd);
                }
                std::cout << "HTTP/1.1 200 OK\r\n";
                std::vector<std::string> emptyARGV;
                std::string fpath = std::string(getenv("DOCUMENT_ROOT")).append(getenv("REQUEST_URI"));
                if(execvp(fpath.c_str(), vecStrToChar(emptyARGV)) == -1){
                  std::cerr << "execvp: " << strerror(errno) << std::endl;
                  exit(0);
                }
              }
              else{ //parent
                socket_.close();
              }
              
            }
            /* console.cgi */
            else if (isConsole){
              std::vector<struct shConn> shConnVec;

              std::istringstream issQry(getenv("QUERY_STRING"));
              std::string itmInQry;
              std::string itmNoHead;
              for(int iConn = 0; iConn < 5; iConn++){
                /* nplinux1.cs.nctu.edu.tw */
                struct shConn tmp;
                shConnVec.push_back(tmp);

                std::getline(issQry, itmInQry, '&');
                itmNoHead = itmInQry.substr(itmInQry.find_first_of("=")+1);
                shConnVec[iConn].shHost = itmNoHead;
                /* 1234 */
                std::getline(issQry, itmInQry, '&');
                itmNoHead = itmInQry.substr(itmInQry.find_first_of("=")+1);
                shConnVec[iConn].shPort = atoi(itmNoHead.c_str());
                /* t1.txt */
                std::getline(issQry, itmInQry, '&');
                itmNoHead = itmInQry.substr(itmInQry.find_first_of("=")+1);
                shConnVec[iConn].cmdFile = itmNoHead;

                display(shConnVec[iConn]);
              }
              
              // do_write(length);
            }
            else{
              // error 404
              memset(wdata_, '\0', sizeof(wdata_));
              strcpy(wdata_, "HTTP/1.1 404 Not Found\r\n\r\n");
              do_write(strlen(wdata_));

            }
          }
        });
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(wdata_, length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            // do_read();
          }
        });
  }

  

  tcp::socket socket_;
  enum { max_length = 4096 };
  char rdata_[max_length];
  char wdata_[max_length];
  bool isPanel = false;
  bool isConsole = false;
  struct shConn{
    std::string shHost;
    int shPort;
    std::string cmdFile;
  };
  void display(struct shConn conn){
    std::cout << "Host: \t" << conn.shHost << std::endl;
    std::cout << "Port: \t" << conn.shPort << std::endl;
    std::cout << "File: \t" << conn.cmdFile << std::endl;
  }
  char** vecStrToChar(std::vector<std::string> cmd)
  {
    char** result = (char**)malloc(sizeof(char*)*(cmd.size()+1));
    for(size_t i = 0; i < cmd.size(); i++){
      result[i] = strdup(cmd[i].c_str());
    }
    result[cmd.size()] = NULL;
    return result;
  }
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}