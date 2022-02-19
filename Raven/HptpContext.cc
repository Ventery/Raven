#include <assert.h>

#include "HptpContext.h"

namespace Raven
{
	//static
	std::string HptpContext::makeMessage(const std::string &msg, const std::string &key, const std::string &iv, const HPTPMessageType &textType, const Dict &addtionHeaders)
	{
		std::string message = "HPTP/1.0 " + std::to_string((int)textType) + "\r\n";
		std::string text = msg;

		switch (textType)
		{
		case PLAINTEXT:
		case PLAINTEXT_WINCTL:
		case TRANSFER:
		{ // treat TRANSFER message as plaintext
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
		} //end switch

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
				return PARSE_ERROR;
			}
		}

		case STATE_PARSE_HEADERS:
		{
			HeaderState state = parseHeader();
			if (state == PARSE_HEADER_SUCCESS)
			{
				sockInfo_.sockState = STATE_CIPHERTEXT;
			}
			else if (state == PARSE_HEADER_AGAIN)
			{
				return PARSE_AGAIN;
			}
			else
			{
				return PARSE_ERROR;
			}
		}

		case STATE_CIPHERTEXT:
		{
			CiphertextState state = parseCiphertext();
			if (state == CIPHERTEXT_SUCCESS)
			{
				sockInfo_.sockState = STATE_PARSE_PROTOCOL;
				if (sockInfo_.textType == TRANSFER)
				{
					return PARSE_SUCCESS_TRANSFER;
				}
				else
				{
					return PARSE_SUCCESS;
				}
			}
			else if (state == CIPHERTEXT_AGAIN)
			{
				return PARSE_AGAIN;
			}
			else
			{
				return PARSE_ERROR;
			}
		}

		default:
		{
			formatTime("Program should not come here!\n");
			return PARSE_ERROR;
		}
		} //end switch
	}

	ProtocolState HptpContext::parseProtocol()
	{
		std::string &str = sockInfo_.readBuffer;
		size_t posEnd = str.find(std::string("\r\n"));
		if (posEnd == std::string::npos)
		{
			return PARSE_PROTOCOL_AGAIN;
		}
		size_t posProtocol = str.rfind("HPTP/1.0", posEnd);
		if (posProtocol == std::string::npos)
		{
			return PARSE_PROTOCOL_ERROR;
		}
		if (posEnd - posProtocol != 12)
		{
			return PARSE_PROTOCOL_ERROR;
		}
		sockInfo_.textType = (HPTPMessageType)stoi(str.substr(posProtocol + 9, 3));
		str = str.substr(posEnd + 2);
		if (sockInfo_.textType == KEEPALIVE)
		{
			return PARSE_PROTOCOL_SUCCESS_KEEPALIVE;
		}
		else
		{
			return PARSE_PROTOCOL_SUCCESS;
		}
	}

	HeaderState HptpContext::parseHeader()
	{
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
				return PARSE_HEADER_AGAIN;
			size_t posMid = str.rfind(":", posLineEnd);
			if (posMid > 0 && posLineEnd > posMid + 2)
			{
				std::string key = str.substr(0, posMid);
				std::string value = str.substr(posMid + 2, posLineEnd - posMid - 2);
				//cout << key << " " << value << endl;
				sockInfo_.headers[key] = value;
				str = str.substr(posLineEnd + 2);
			}
			else
			{
				return PARSE_HEADER_ERROR;
			}
		}
		return PARSE_HEADER_SUCCESS;
	}

	//check the length
	CiphertextState HptpContext::parseCiphertext()
	{
		int cipherLength = atoi((sockInfo_.headers["length"]).c_str());
		if (cipherLength <= 0)
		{
			return CIPHERTEXT_ERROR;
		}
		int trueLength;
		if (sockInfo_.textType == PLAINTEXT || sockInfo_.textType == PLAINTEXT_WINCTL || sockInfo_.textType == TRANSFER)
		{
			trueLength = cipherLength;
		}
		else
		{
			trueLength = (cipherLength % kBlockSize == 0 ? cipherLength : ((cipherLength) / kBlockSize + 1) * kBlockSize); //encrypted text
		}

		if (sockInfo_.readBuffer.size() < (unsigned int)(trueLength + 2))
		{
			return CIPHERTEXT_AGAIN;
		}

		if (sockInfo_.readBuffer.c_str()[trueLength] != '\r' || sockInfo_.readBuffer.c_str()[trueLength + 1] != '\n')
		{
			return CIPHERTEXT_ERROR;
		}

		sockInfo_.payload = sockInfo_.readBuffer.substr(0, trueLength);
		sockInfo_.readBuffer = sockInfo_.readBuffer.substr(trueLength + 2);
		return CIPHERTEXT_SUCCESS;
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
} //namespace Raven