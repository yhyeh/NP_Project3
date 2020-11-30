/* compile */
// g++ cgi_server.cpp -o cgi_server -lws2_32 -lwsock32 -std=c++14

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

#include <boost/asio.hpp>
//#include <boost/thread.hpp>

using boost::asio::ip::tcp;

/* global */
std::string panelHtml;

/* struct */
struct shConn{
  std::string shHost;
  std::string shPort;
  std::string cmdFile;
};

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
    memset(rdata_, '\0', max_length);
  }

  void start()
  {
    do_read();
  }

private:
  //boost::thread panelThread;
  //boost::thread consoleThread;
  tcp::socket socket_;
  enum { max_length = 4096 };
  char rdata_[max_length];
  //char wdata_[max_length];
  std::vector<struct shConn> shConnVec;
  std::string tmpl;
  std::string REQUEST_METHOD;
  std::string REQUEST_URI;
  std::string QUERY_STRING;
  std::string SERVER_PROTOCOL;
  std::string HTTP_HOST;
  std::string SERVER_ADDR;
  std::string SERVER_PORT;
  std::string REMOTE_ADDR;
  std::string REMOTE_PORT;
  /* function */
  std::vector<struct shConn> parseQry(std::string);
  void getTemplate(std::string& tmpl, std::vector<struct shConn> shConnVec);

  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(rdata_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec){
            /* view request */
            std::string req(rdata_);
            memset(rdata_, '\0', max_length);
            std::cout << req << std::endl;
            /* parse request */
            parseReq(req);
            std::cout << "REQUEST_URI: " << REQUEST_URI << std::endl;
            /* panel or console */
            if (REQUEST_URI == "/panel.cgi")
            {
              panelCGI();
            }
            else if (REQUEST_URI == "/console.cgi")
            {
              consoleCGI();
            }
            else
            {
              std::string wstr = "HTTP/1.1 404 Not Found\r\n\r\n";
              auto self(shared_from_this());
              boost::asio::async_write(socket_, boost::asio::buffer(wstr, wstr.size()),
                  [this, self](boost::system::error_code ec, std::size_t /*length*/)
                  {
                    if (!ec)
                    {
                      this->socket_.close();
                    }
                  });
            }
          }
        });
  }
  /* CGI function */
  void consoleCGI()
  {
    shConnVec = parseQry(QUERY_STRING);
    getTemplate(tmpl, shConnVec);
    /* render html */
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(tmpl, tmpl.size()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            size_t s;
            try
            {
              boost::asio::io_context io_context;
              for (s = 0; s < 5; s++){
                if (shConnVec[s].shHost == "") continue;
                //std::cout << "start session: " << s << std::endl;
                std::make_shared<ShellSession>(io_context, socket_)->start(shConnVec[s], (int)s);
              }
              io_context.run();
            }
            catch (std::exception& e)
            {
              std::cout << e.what() << std::endl;
              //output_command(s, std::string(e.what())+"\n");
            }
          }
        });
    
    
  }
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

  class ShellSession
    : public std::enable_shared_from_this<ShellSession> 
  { 
  private:
    tcp::socket shsocket_;
    tcp::socket& websocket_;
    tcp::resolver resolver_;
    tcp::resolver::results_type endpoints;
    int session;
    
    std::string input_buffer_;
    enum { max_length = 4096 };
    char rbuf_[max_length];

    struct shConn shInfo;
    std::vector<std::string> cmdVec;
    std::string cmd;
    std::string htmlContent;

    bool stopped_ = false;
    
    std::string html_escape(std::string content);


  public:
    ShellSession(boost::asio::io_context& io_context, tcp::socket& socket_)
      : shsocket_(io_context),
        resolver_(io_context),
        websocket_(socket_)
    {
      memset(rbuf_, '\0', max_length);
      stopped_ = false;
    }
    void start(struct shConn shConn, int sessionID) 
    {
      shInfo = shConn;
      session = sessionID;

      std::cout << "constructor session: " << session << std::endl;
      std::cout << "shInfo.shHost: " << shInfo.shHost << std::endl;
      std::cout << "shInfo.shPort: " << shInfo.shPort << std::endl;
      
      std::fstream fs("./test_case/" + shInfo.cmdFile);
      std::string lineInCmd;
      while(std::getline(fs, lineInCmd)){
        lineInCmd.append("\n");
        cmdVec.push_back(lineInCmd);
      }
      fs.close();
      /*
      for (size_t i = 0; i < cmdVec.size(); i++){
        std::cout << i << "\t: " << cmdVec[i] << std::flush;
      }
      */
      do_resolve();
    }
    void stop() 
    {
      stopped_ = true;
      std::cout << "destructor session: " << session << std::endl;
      shsocket_.close();
    }
  private:  
    void do_resolve() {
      if (stopped_) return;
      auto self(shared_from_this());
      resolver_.async_resolve(shInfo.shHost, shInfo.shPort,
        [this, self](boost::system::error_code ec,
        tcp::resolver::results_type returned_endpoints)
        {
          if (stopped_) return;
          if (!ec)
          {
            endpoints = returned_endpoints;
            tcp::resolver::results_type::iterator endpoint_iter = endpoints.begin();
            while(endpoint_iter != endpoints.end())
            {
              //std::cout << "Trying " << endpoint_iter->endpoint() << "\n";
              endpoint_iter++;
            }
            do_connect();
          }
          else{
            std::cout << "resolve Error: " << ec.message() << "\n";
            stop();
          }
        });
    }
    void do_connect() { // get socket after connect
      if (stopped_) return;
      auto self(shared_from_this());
      boost::asio::async_connect(shsocket_, endpoints,
      [this, self](boost::system::error_code ec, tcp::endpoint){
        if (stopped_) return;
        if (!ec)
        {
          do_send_cmd(); // for test in echo server
          //do_read();
        }
        else{
          std::cout << "connect Error: " << ec.message() << "\n";
          stop();
        }
      });
    }
    void do_read() 
    {
      if (stopped_) return;
      std::cout << "this is do read" << std::endl;


      auto self(shared_from_this());
      /*
      boost::asio::async_read_until(shsocket_,
          boost::asio::dynamic_buffer(input_buffer_), "% ",
          [this, self](boost::system::error_code ec, std::size_t length)
          {
            if (stopped_) return;
            if (!ec){
              std::string reply(input_buffer_.substr(0, length - 2));
              input_buffer_.erase(0, length);
              reply.append("% ");
              std::cout << reply << std::flush;
              //output_shell(session, reply);
              htmlContent.clear();
              htmlContent.append("<script>document.getElementById('s");
              htmlContent.append(std::to_string(session));
              htmlContent.append("').innerHTML += '");
              htmlContent.append(html_escape(reply));
              htmlContent.append("';</script>");
              boost::asio::async_write(websocket_, boost::asio::buffer(htmlContent.c_str(), htmlContent.size()),
                  [this, self](boost::system::error_code ec, std::size_t )
                  {
                    if (stopped_) return;
                    if (!ec)
                    {
                      do_send_cmd();
                    }
                    else
                    {
                      std::cout << "output shell Error: " << ec.message() << std::endl;
                      stop();
                    }
                    
                  });
            }else{
              if (ec == boost::asio::error::eof){
                std::cout << "get eof" << std::endl;
                stop();
              }else{
                std::cout << "read golden reply Error: " << ec.message() << std::endl;

              }
            }
          });
          */
      shsocket_.async_read_some(
          boost::asio::buffer(rbuf_, max_length-1), 
          [this, self](boost::system::error_code ec, std::size_t length)
          {
            if (stopped_) return;
            if (!ec){
              rbuf_[length+1] = '\0';
              std::cout << "(raw recv "<< length <<")" << rbuf_ << std::endl;
              
              std::string reply = std::string(rbuf_).substr(0, length);
              memset(rbuf_, '\0', max_length);
              //output_shell(session, reply);
              std::cout << "(recv)" << reply << std::flush;
              /*
              if (reply.find("% ") != std::string::npos){
                //std::cout << "get %" << std::endl;
                do_send_cmd();
              }
              */
              do_send_cmd(); // for test in echo server

              do_read();
            }
            else{
              std::cout << "read Error: " << ec.message() << std::endl;
              if (ec == boost::asio::error::eof){
                std::cout << "get eof" << std::endl;
                stop();
              }
            }
          });
    }
    void do_send_cmd() {
      if (stopped_) return;
      // get cmd from .txt
      cmd = cmdVec.front();
      cmdVec.erase(cmdVec.begin());
      //output_command(session, cmd);
      htmlContent.clear();
      htmlContent.append("<script>document.getElementById('s");
      htmlContent.append(std::to_string(session));
      htmlContent.append("').innerHTML += '<b>");
      htmlContent.append(html_escape(cmd));
      htmlContent.append("</b>';</script>");
      
      // send cmd to shell server
      std::cout << cmd << std::flush;
      
      auto self(shared_from_this());
      boost::asio::async_write(shsocket_, boost::asio::buffer(cmd.c_str(), cmd.size()),
          [this, self](boost::system::error_code ec, std::size_t )
          {
            if (stopped_) return;
            if (!ec)
            {
              std::cout << "send cmd to shell server success" <<  std::endl;
              // send cmd to usr
              std::cout << htmlContent << std::endl;
              
              auto self(shared_from_this());
              boost::asio::async_write(websocket_, boost::asio::buffer(htmlContent.c_str(), htmlContent.size()),
                  [this, self](boost::system::error_code ec, std::size_t )
                  {
                    if (stopped_) return;
                    if (!ec)
                    {
                      std::cout << "send cmd to usr success" <<  std::endl;
                      if (cmd == "exit\n"){
                        stop();
                      }
                      else{
                        do_read();
                      }
                    }
                    else{
                      std::cout << "output cmd Error: " << ec.message() << std::endl;
                      stop();
                    }
                  });
              
            }
            else{
              std::cout << "write cmd to golden Error: " << ec.message() << std::endl;
              stop();
            }
          });
      
      


    }
    void send_cmd_to_server()
    {
      if (stopped_) return;
      
      
    }
  };

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
    //std::cout << "hello" << std::endl;
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
  //panelHtml.append("<text>hello</text>");
  
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
      <form action="console.cgi" method="GET">
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


/*
void session::output_shell(int session, std::string content){
  std::string htmlContent(content);
  html_escape(htmlContent);
  std::cout << "<script>document.getElementById('s" << session << "').innerHTML += '" << htmlContent << "';</script>" << std::flush;
}

void session::output_command(int session, std::string content){
  std::string htmlContent(content);
  html_escape(htmlContent);
  std::cout << "<script>document.getElementById('s" << session << "').innerHTML += '<b>" << htmlContent <<"</b>';</script>" << std::flush;
}
*/

std::string session::ShellSession::html_escape(std::string content){
  std::string newContent;
  for(size_t i = 0; i < content.size(); i++){
    std::string curChar = content.substr(i, 1);
    if(curChar == "&") newContent.append("&amp;");
    else if(curChar == ">") newContent.append("&gt;");
    else if(curChar == "<") newContent.append("&lt;");
    else if(curChar == "\"") newContent.append("&quot;");
    else if(curChar == "\'") newContent.append("&apos;");
    else if(curChar == "\n") newContent.append("&NewLine;");
    else if(curChar == "\r") newContent.append("");
    else newContent.append(curChar);
  }
  return newContent;
}

std::vector<struct shConn> session::parseQry(std::string QUERY_STRING){
  std::vector<struct shConn> shConnVec;

  std::istringstream issQry(QUERY_STRING);
  std::string itmInQry;
  std::string itmNoHead;
  for(int iConn = 0; iConn < 5; iConn++){
    struct shConn tmp;
    shConnVec.push_back(tmp);
    /* nplinux1.cs.nctu.edu.tw */
    getline(issQry, itmInQry, '&');
    itmNoHead = itmInQry.substr(itmInQry.find_first_of("=")+1);
    shConnVec[iConn].shHost = itmNoHead;
    /**************************************************/
    if (itmNoHead != ""){
      //shConnVec[iConn].shHost = "nplinux9.cs.nctu.edu.tw";
      shConnVec[iConn].shHost = "localhost";
    }
    /**************************************************/
    /* 1234 */
    getline(issQry, itmInQry, '&');
    itmNoHead = itmInQry.substr(itmInQry.find_first_of("=")+1);
    shConnVec[iConn].shPort = itmNoHead;
    /* t1.txt */
    getline(issQry, itmInQry, '&');
    itmNoHead = itmInQry.substr(itmInQry.find_first_of("=")+1);
    shConnVec[iConn].cmdFile = itmNoHead;
  }
  return shConnVec;
}

void session::getTemplate(std::string& tmpl, std::vector<struct shConn>shConnVec){
  tmpl = 
  "HTTP/1.1 200 OK\r\n"
  "Content-type: text/html\r\n\r\n"
  "<!DOCTYPE html>"
  "<html lang=\"en\">"
    "<head>"
      "<meta charset=\"UTF-8\" />"
      "<title>NP Project 3 Sample Console</title>"
      "<link "
        "rel=\"stylesheet\""
        "href=\"https://cdn.jsdelivr.net/npm/bootstrap@4.5.3/dist/css/bootstrap.min.css\""
        "integrity=\"sha384-TX8t27EcRE3e/ihU7zmQxVncDAy5uIKz4rEkgIXeMed4M0jlfIDPvg6uqKI2xXr2\""
        "crossorigin=\"anonymous\""
      "/>"
      "<link "
        "href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\""
        "rel=\"stylesheet\""
      "/>"
      "<link "
        "rel=\"icon\""
        "type=\"image/png\""
        "href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\""
      "/>"
      "<style>"
        "* {"
          "font-family: 'Source Code Pro', monospace;"
          "font-size: 1rem !important;"
        "}"
        "body {"
          "background-color: #212529;"
        "}"
        "pre {"
          "color: #cccccc;"
        "}"
        "b {"
          "color: #01b468;"
        "}"
      "</style>"
    "</head>"
    "<body>"
      "<table class=\"table table-dark table-bordered\">"
        "<thead>"
          "<tr>"
            "<th scope=\"col\">";
  if (shConnVec[0].shHost != "")
    tmpl.append(shConnVec[0].shHost+":"+shConnVec[0].shPort);
  tmpl.append(
            "</th>"
            "<th scope=\"col\">"
  );
  if (shConnVec[1].shHost != "")
   tmpl.append(shConnVec[1].shHost+":"+shConnVec[1].shPort);
  tmpl.append(
            "</th>"
          "</tr>"
        "</thead>"
        "<tbody>"
          "<tr>"
            "<td><pre id=\"s0\" class=\"mb-0\"></pre></td>"
            "<td><pre id=\"s1\" class=\"mb-0\"></pre></td>"
          "</tr>"
        "</tbody>"
      "</table>"
      "<table class=\"table table-dark table-bordered\">"
        "<thead>"
          "<tr>"
            "<th scope=\"col\">"
  );
  if (shConnVec[2].shHost != "")
    tmpl.append(shConnVec[2].shHost+":"+shConnVec[2].shPort);
  tmpl.append(
            "</th>"
            "<th scope=\"col\">"
  );
  if (shConnVec[3].shHost != "")
    tmpl.append(shConnVec[3].shHost+":"+shConnVec[3].shPort);
  tmpl.append(
            "</th>"
          "</tr>"
        "</thead>"
        "<tbody>"
          "<tr>"
            "<td><pre id=\"s2\" class=\"mb-0\"></pre></td>"
            "<td><pre id=\"s3\" class=\"mb-0\"></pre></td>"
          "</tr>"
        "</tbody>"
      "</table>"
      "<table class=\"table table-dark table-bordered\">"
        "<thead>"
          "<tr>"
            "<th scope=\"col\">"
  );
  if (shConnVec[4].shHost != "")
    tmpl.append(shConnVec[4].shHost+":"+shConnVec[4].shPort);
  tmpl.append(
            "</th>"
          "</tr>"
        "</thead>"
        "<tbody>"
          "<tr>"
            "<td><pre id=\"s4\" class=\"mb-0\"></pre></td>"
          "</tr>"
        "</tbody>"
      "</table>"
    "</body>"
  "</html>"
  );
}

