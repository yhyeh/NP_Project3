#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <functional>

using boost::asio::steady_timer;
using boost::asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

/* struct */
struct shConn{
  std::string shHost;
  int shPort;
  std::string cmdFile;
};

/* function */
std::vector<struct shConn> parseQry();
void output_shell(int session, std::string content);
void output_command(int session, std::string content);
void html_escape(std::string &content);
void getTemplate(std::string& tmpl, std::vector<struct shConn> shConnVec);

int main(){
  std::vector<struct shConn> shConnVec = parseQry();
  std::string tmpl;
  getTemplate(tmpl, shConnVec);
  /* render html */
  std::cout << "Content-type: text/html\r\n\r\n" << std::flush;
  std::cout << tmpl << std::flush;
  
}

std::vector<struct shConn> parseQry(){
  std::vector<struct shConn> shConnVec;

  std::istringstream issQry(getenv("QUERY_STRING"));
  std::string itmInQry;
  std::string itmNoHead;
  for(int iConn = 0; iConn < 5; iConn++){
    struct shConn tmp;
    shConnVec.push_back(tmp);
    /* nplinux1.cs.nctu.edu.tw */
    getline(issQry, itmInQry, '&');
    itmNoHead = itmInQry.substr(itmInQry.find_first_of("=")+1);
    shConnVec[iConn].shHost = itmNoHead;
    /* 1234 */
    getline(issQry, itmInQry, '&');
    itmNoHead = itmInQry.substr(itmInQry.find_first_of("=")+1);
    shConnVec[iConn].shPort = atoi(itmNoHead.c_str());
    /* t1.txt */
    getline(issQry, itmInQry, '&');
    itmNoHead = itmInQry.substr(itmInQry.find_first_of("=")+1);
    shConnVec[iConn].cmdFile = itmNoHead;
  }
  return shConnVec;
}

void output_shell(int session, std::string content){
  std::string htmlContent(content);
  html_escape(htmlContent);
  std::cout << "<script>document.getElementById('s" << session << "').innerHTML += '" << htmlContent << "';</script>" << std::flush;
}

void output_command(int session, std::string content){
  std::string htmlContent(content);
  html_escape(htmlContent);
  std::cout << "<script>document.getElementById('s" << session << "').innerHTML += '<b>" << htmlContent <<"</b>';</script>" << std::flush;
}

void html_escape(std::string &content){
  for(size_t i = 0; i < content.size(); i++){
    char curChar = content[i];
    if(curChar == '&') content.replace(i, 1, "&amp;");
    else if(curChar == '>') content.replace(i, 1, "&gt;");
    else if(curChar == '<') content.replace(i, 1, "&lt;");
    else if(curChar == '\"') content.replace(i, 1, "&quot;");
    else if(curChar == '\'') content.replace(i, 1, "&apos;");
    else if(curChar == '\n') content.replace(i, 1, "&NewLine;");
  }
}

void getTemplate(std::string& tmpl, std::vector<struct shConn>shConnVec){
  tmpl = 
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
  tmpl.append(shConnVec[0].shHost+":"+std::to_string(shConnVec[0].shPort));
  tmpl.append(
            "</th>"
            "<th scope=\"col\">"
  );
  tmpl.append(shConnVec[1].shHost+":"+std::to_string(shConnVec[1].shPort));
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
  tmpl.append(shConnVec[2].shHost+":"+std::to_string(shConnVec[2].shPort));
  tmpl.append(
            "</th>"
            "<th scope=\"col\">"
  );
  tmpl.append(shConnVec[3].shHost+":"+std::to_string(shConnVec[3].shPort));
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
  tmpl.append(shConnVec[4].shHost+":"+std::to_string(shConnVec[4].shPort));
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