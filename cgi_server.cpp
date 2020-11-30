/* compile */
// g++ cgi_server.cpp -o cgi_server -lws2_32 -lwsock32 -std=c++14

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

using boost::asio::ip::tcp;

/* global */
std::string panelHtml;

/* function */
void getPanelHtml();

/* class */
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
  boost::thread panelThread;
  boost::thread consoleThread;
  tcp::socket socket_;
  enum { max_length = 4096 };
  char rdata_[max_length];
  //char wdata_[max_length];
  std::string REQUEST_METHOD;
  std::string REQUEST_URI;
  std::string QUERY_STRING;
  std::string SERVER_PROTOCOL;
  std::string HTTP_HOST;
  std::string SERVER_ADDR;
  std::string SERVER_PORT;
  std::string REMOTE_ADDR;
  std::string REMOTE_PORT;
  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(rdata_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec){
            /* view request */
            std::string req(rdata_);
            std::cout << req << std::endl;
            /* parse request */
            parseReq(req);
            /* panel or console */
            if (REQUEST_URI == "/panel.cgi")
            {
              panelCGI();
            }
            else if (REQUEST_URI == "/console.cgi")
            {

            }
            else
            {

            }
          }
        });
  }
  /* CGI function */
  void panelCGI()
  {
    //std::cout << "panelThread" << std::endl;
    //std::cout << "panelHtml:" << panelHtml << std::endl;

    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(panelHtml.c_str(), panelHtml.size()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            this->socket_.close();
          }
        });

  }
  /* util function */
  void parseReq(std::string req) /* store all env as string */
  {
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
            REQUEST_METHOD = wordInLine;
          }
          else if (iWord == 1){
            /* /panel.cgi */
            /* /console.cgi?h0=nplinux1.cs.nctu.edu.tw&p0= ... */
            std::string file = wordInLine.substr(1, wordInLine.find_first_of(".")-1);
            file = "/" + file + ".cgi";
            REQUEST_URI = file;
            if (wordInLine.find_first_of("?") == std::string::npos){
              QUERY_STRING = "";
            }else {
              QUERY_STRING = wordInLine.substr(wordInLine.find_first_of("?")+1);
            }
          }
          else{
            /* HTTP/1.1 */
            SERVER_PROTOCOL = wordInLine;
          }
        }
      }
      else if (iLine == 1){
        /* Host: localhost:18787 */
        HTTP_HOST = lineInReq.substr(6);
      }             
    }
    /* set other env */
    SERVER_ADDR = socket_.local_endpoint().address().to_string();
    SERVER_PORT = std::to_string(socket_.local_endpoint().port());
    REMOTE_ADDR = socket_.remote_endpoint().address().to_string();
    REMOTE_PORT = std::to_string(socket_.remote_endpoint().port());
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
    std::cout << "hello" << std::endl;
    try
    {
        if (argc != 2)
        {
        std::cerr << "Usage: http_server <port>\n";
        return 1;
        }
        getPanelHtml();

        boost::asio::io_context io_context;

        server s(io_context, std::atoi(argv[1]));

        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    system("pause");
    return 0;
}

void getPanelHtml(){
  /* global string: panelHtml */
  panelHtml.append("HTTP/1.1 200 OK\r\n");
  panelHtml.append("Content-type: text/html\r\n\r\n");
  panelHtml.append("<text>hello</text>");
  
  panelHtml.append(R"XXXX(<!DOCTYPE html>
  <html lang="en">
    <head>
      <title>NP Project 3 Panel</title>
      <link
        rel="stylesheet"
        href="https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css"
        integrity="sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2"
        crossorigin="anonymous"
      />
      <link
        href="https://fonts.googleapis.com/css?family=Source+Code+Pro"
        rel="stylesheet"
      />
      <link
        rel="icon"
        type="image/png"
        href="https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png"
      />
      <style>
        * {
          font-family: 'Source Code Pro', monospace;
        }
      </style>
    </head>
    <body class="bg-secondary pt-5">
      <form action="GET" method="console.cgi">
        <table class="table mx-auto bg-light" style="width: inherit">
          <thead class="thead-dark">
            <tr>
              <th scope="col">#</th>
              <th scope="col">Host</th>
              <th scope="col">Port</th>
              <th scope="col">Input File</th>
            </tr>
          </thead>
          <tbody>)XXXX");
  for(int i = 0; i < 5; i++)
  {
    std::string id = std::to_string(i);
    panelHtml.append(R"XXXX(
          <tr>
            <th scope="row" class="align-middle">Session )XXXX"+std::to_string(i+1)+R"XXXX(</th>
            <td>
              <div class="input-group">
                <select name="h)XXXX"+id+R"XXXX(" class="custom-select">
                  <option></option>
                  <option value="nplinux1.cs.nctu.edu.tw">nplinux1</option>
                  <option value="nplinux2.cs.nctu.edu.tw">nplinux2</option>
                  <option value="nplinux3.cs.nctu.edu.tw">nplinux3</option>
                  <option value="nplinux4.cs.nctu.edu.tw">nplinux4</option>
                  <option value="nplinux5.cs.nctu.edu.tw">nplinux5</option>
                  <option value="nplinux6.cs.nctu.edu.tw">nplinux6</option>
                  <option value="nplinux7.cs.nctu.edu.tw">nplinux7</option>
                  <option value="nplinux8.cs.nctu.edu.tw">nplinux8</option>
                  <option value="nplinux9.cs.nctu.edu.tw">nplinux9</option>
                  <option value="nplinux10.cs.nctu.edu.tw">nplinux10</option>
                  <option value="nplinux11.cs.nctu.edu.tw">nplinux11</option>
                  <option value="nplinux12.cs.nctu.edu.tw">nplinux12</option>
                </select>
                <div class="input-group-append">
                  <span class="input-group-text">.cs.nctu.edu.tw</span>
                </div>
              </div>
            </td>
            <td>
              <input name="p)XXXX"+id+R"XXXX(" type="text" class="form-control" size="5" />
            </td>
            <td>
              <select name="f)XXXX"+id+R"XXXX(" class="custom-select">
                <option></option>
                <option value="t1.txt">t1.txt</option>
                <option value="t2.txt">t2.txt</option>
                <option value="t3.txt">t3.txt</option>
                <option value="t4.txt">t4.txt</option>
                <option value="t5.txt">t5.txt</option>
              </select>
            </td>
          </tr>)XXXX");
  }
  
  panelHtml.append(R"XXXX(
            <tr>
              <td colspan="3"></td>
              <td>
                <button type="submit" class="btn btn-info btn-block">Run</button>
              </td>
            </tr>
          </tbody>
        </table>
      </form>
    </body>
  </html>)XXXX");
  
}


