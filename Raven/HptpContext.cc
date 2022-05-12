#include <assert.h>

#include "HptpContext.h"

#include <fstream>
#include <iostream>
using namespace std;


namespace Raven
{
	// static
	std::string HptpContext::makeMessage(const std::string &msg, const std::string &key, const std::string &iv, const HPTPMessageType &textType, const Dict &addtionHeaders)
	{
		std::string message = "HPTP/1.0 " + std::to_string((int)textType) + "\r\n";
		std::string text = msg;

		switch (textType)
		{
		case PLAINTEXT:
		case PLAINTEXT_WINCTL:
		case TRANSFER:
		case FILETRANSFER:
		{ // treat FILETRANSFER message as plaintext
			message += "length: " + std::to_string(msg.size()) + "\r\n";
			break;
		}

		case CIPHERTEXT:
		{
			message += "iv: " + iv + "\r\n";
			message += "length: " + std::to_string(msg.size()) + "\r\n";
			text = encode(msg, key, iv);
			break;
		}

		case KEEPALIVE:
		{
			break;
		}

		default:
		{
			break;
		}
		} // end switch

		if (!addtionHeaders.empty())
		{
			for (auto it : addtionHeaders)
			{
				message += it.first + ": " + it.second + "\r\n";
			}
		}

		message += "\r\n" + text + "\r\n";
		return message;
	}

	MessageState HptpContext::parseMessage()
	{
		//std::cout<<sockInfo_.readBuffer.length()<<std::endl;
		//std::cout<<sockInfo_.readBuffer<<std::endl;

		switch (sockInfo_.sockState)
		{
		case STATE_PARSE_PROTOCOL:
		{
			ProtocolState state = parseProtocol();
			if (state == PARSE_PROTOCOL_SUCCESS)
			{
				sockInfo_.sockState = STATE_PARSE_HEADERS;
				sockInfo_.headers.clear();
			}
			else if (state == PARSE_PROTOCOL_AGAIN)
			{
				return PARSE_AGAIN;
			}
			else if (state == PARSE_PROTOCOL_SUCCESS_KEEPALIVE)
			{
				return PARSE_SUCCESS_KEEPALIVE;
			}
			else
			{
				return PARSE_ERROR_PROTOCOL;
			}
		}

		case STATE_PARSE_HEADERS:
		{
			HeaderState state = parseHeader();
			if (state == PARSE_HEADER_SUCCESS)
			{
				sockInfo_.sockState = STATE_PARSE_TEXT;
			}
			else if (state == PARSE_HEADER_AGAIN)
			{
				return PARSE_AGAIN;
			}
			else
			{
				return PARSE_ERROR_HEADER;
			}
		}

		case STATE_PARSE_TEXT:
		{
			TextState state = parseText();
			if (state == PARSE_TEXT_SUCCESS)
			{
				sockInfo_.sockState = STATE_PARSE_PROTOCOL;
				if (sockInfo_.textType == CIPHERTEXT)
				{
					return parseMessage();
				}
				else if (sockInfo_.textType == TRANSFER)
				{
					return PARSE_SUCCESS_TRANSFER;
				}
				else
				{
					return PARSE_SUCCESS;
				}
			}
			else if (state == PARSE_TEXT_AGAIN)
			{
				return PARSE_AGAIN;
			}
			else
			{
				//std::cout << sockInfo_.readBuffer << std::endl;
				//std::cout << sockInfo_.readBuffer.length() << std::endl;
				return PARSE_ERROR_TEXT;
			}
		}

		default:
		{
			formatTime("Program should not come here!\n");
			return PARSE_ERROR_PROTOCOL;
		}
		} // end switch
	}

	ProtocolState HptpContext::parseProtocol()
	{
		// std::cout<<"parseProtocol"<<std::endl;
		std::string &str = sockInfo_.readBuffer;
		size_t posEnd = str.find(std::string("\r\n"));
		if (posEnd == std::string::npos)
		{
			//std::cout << "PARSE_PROTOCOL_AGAIN sockInfo_.readBuffer.length():" << sockInfo_.readBuffer.length() << std::endl;
			return PARSE_PROTOCOL_AGAIN;
		}
		size_t posProtocol = str.rfind("HPTP/1.0", posEnd);
		if (posProtocol == std::string::npos || posEnd - posProtocol != 12)
		{
			//std::cout << "PARSE_PROTOCOL_ERROR" << std::endl;
			return PARSE_PROTOCOL_ERROR;
		}
		sockInfo_.textType = (HPTPMessageType)stoi(str.substr(posProtocol + 9, 3));
		if (sockInfo_.textType == KEEPALIVE)
		{
			str = str.substr(posEnd + 6);
			return PARSE_PROTOCOL_SUCCESS_KEEPALIVE;
		}
		else
		{
			str = str.substr(posEnd + 2);
			return PARSE_PROTOCOL_SUCCESS;
		}
	}

	HeaderState HptpContext::parseHeader()
	{
		// std::cout<<"parseHeader"<<std::endl;
		std::string &str = sockInfo_.readBuffer;
		while (true)
		{
			size_t posLineEnd = str.find(std::string("\r\n"));
			if (posLineEnd == 0) // header is over
			{
				str = str.substr(2);
				break;
			}
			if (posLineEnd == std::string::npos)
			{
				//std::cout << "PARSE_HEADER_AGAIN str:" << str << std::endl;
				return PARSE_HEADER_AGAIN;
			}
			size_t posMid = str.rfind(":", posLineEnd);
			if (posMid > 0 && posLineEnd > posMid + 2)
			{
				std::string key = str.substr(0, posMid);
				std::string value = str.substr(posMid + 2, posLineEnd - posMid - 2);
				// cout << key << " " << value << endl;
				sockInfo_.headers[key] = value;
				str = str.substr(posLineEnd + 2);
			}
			else
			{
				//std::cout << "PARSE_HEADER_ERROR" << std::endl;
				return PARSE_HEADER_ERROR;
			}
		}
		return PARSE_HEADER_SUCCESS;
	}

	// check the length
	TextState HptpContext::parseText()
	{
		// std::cout<<"parseText"<<std::endl;
		if (sockInfo_.headers.find("length") == sockInfo_.headers.end())
		{
			//std::cout << "PARSE_TEXT_ERROR : length not found!" << std::endl;
			return PARSE_TEXT_ERROR;
		}
		int textLength = atoi((sockInfo_.headers["length"]).c_str());
		int trueLength;
		if (sockInfo_.textType != CIPHERTEXT)
		{
			trueLength = textLength;
		}
		else
		{
			trueLength = (textLength % kBlockSize == 0 ? textLength : ((textLength) / kBlockSize + 1) * kBlockSize); // encrypted text
		}

		if (sockInfo_.readBuffer.size() < (unsigned int)(trueLength + 2))
		{
			//std::cout << "PARSE_TEXT_AGAIN : sockInfo_.readBuffer.size() :" << sockInfo_.readBuffer.size() << std::endl;
			//std::cout << "trueLength : "<<trueLength<<std::endl;
			return PARSE_TEXT_AGAIN;
		}

		if (sockInfo_.readBuffer.c_str()[trueLength] != '\r' || sockInfo_.readBuffer.c_str()[trueLength + 1] != '\n')
		{
			//std::cout << "PARSE_TEXT_ERROR : suffix not found!" << std::endl;
			return PARSE_TEXT_ERROR;
		}

		sockInfo_.payload = sockInfo_.readBuffer.substr(0, trueLength);
		if (sockInfo_.textType == CIPHERTEXT)
		{
            ofstream outfile;
			outfile.open((kFileTransferPath+"log").c_str(),ios::app);

			outfile<<"1---------------------------------"<<std::endl;
			outfile<<sockInfo_.payload<<std::endl;

			outfile<<"2---------------------------------"<<std::endl;
			outfile<<decode(sockInfo_.payload, getAesKey(), sockInfo_.headers["iv"], trueLength)<<std::endl;

			outfile<<"3---------------------------------"<<std::endl;

			sockInfo_.readBuffer = decode(sockInfo_.payload, getAesKey(), sockInfo_.headers["iv"], trueLength) + sockInfo_.readBuffer.substr(trueLength + 2);
		}
		else
		{
			sockInfo_.readBuffer = sockInfo_.readBuffer.substr(trueLength + 2);
		}
		return PARSE_TEXT_SUCCESS;
	}

	int HptpContext::readBlock()
	{
		char rawbuff[MAX_BUFF];
		int readNum = read(sockInfo_.sock, rawbuff, MAX_BUFF);
		sockInfo_.readBuffer += std::string(rawbuff, readNum);
		return readNum;
	}

	int HptpContext::readNoBlock(bool &zero)
	{
		return readn(sockInfo_.sock, sockInfo_.readBuffer, zero);
	}

	int HptpContext::writeBlock(const std::string &message)
	{
		return write(sockInfo_.sock, message.c_str(), message.length());
	}

	int HptpContext::writeNoBlock()
	{
		return writen(sockInfo_.sock, sockInfo_.writeBuffer);
	}

	void HptpContext::pushToWriteBuff(const std::string &message)
	{
		sockInfo_.writeBuffer += message;
	}
} // namespace Raven