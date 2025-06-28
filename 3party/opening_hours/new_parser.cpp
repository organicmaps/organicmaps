#include "new_parser.hpp"
#include <vector>
#include <string>
#include <regex>
#include <iostream>
#include <unordered_map>

namespace osmoh{

const std::vector<std::string> weekdays = {"Su","Mo","Tu","We","Th","Fr","Sa"};
const std::vector<std::string> months = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};


const std::unordered_map<std::string,TokenType> string_to_token_map = {
    

std::vector<Token> tokenize(const std::string& input)
{
  std::string value = input;
  std::vector<Token> tokens;
  size_t pos = 0;
  while(!value.empty())
  {

  } 
}
}
}
int main()
{
      //Input value
    std::string input;
    std::cout<<"Enter the input: ";
    std::getline(std::cin,value);

    std::vector<osmoh::Token> tokens =osmah::tokeize(input);

    for(const auto& token:tokens){
        std::cout<<"type: "<<static_cast<int>(token.type)<<", text: "<<token.text<<", pos: "<<token.pos<<std::endl;
    }
}
