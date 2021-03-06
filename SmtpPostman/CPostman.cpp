#include "CPostman.h"
#include "base64.h"
#include "openssl/err.h"

#include <cassert>

#ifndef LINUX
//Add "openssl-0.9.8l\out32" to Additional Library Directories
//#pragma comment(lib, "ssleay32.lib")
//#pragma comment(lib, "libeay32.lib")
//#pragma comment(lib, "libssl.lib")
//#pragma comment(lib, "libcrypto.lib")
#endif


Command_Entry command_list[] =
{
	{command_INIT,          0,     5 * 60,  220, CExceptionSmtp::SERVER_NOT_RESPONDING},
	{command_EHLO,          5 * 60,  5 * 60,  250, CExceptionSmtp::COMMAND_EHLO},
	{command_AUTHPLAIN,     5 * 60,  5 * 60,  235, CExceptionSmtp::COMMAND_AUTH_PLAIN},
	{command_AUTHLOGIN,     5 * 60,  5 * 60,  334, CExceptionSmtp::COMMAND_AUTH_LOGIN},
	{command_AUTHCRAMMD5,   5 * 60,  5 * 60,  334, CExceptionSmtp::COMMAND_AUTH_CRAMMD5},
	{command_AUTHDIGESTMD5, 5 * 60,  5 * 60,  334, CExceptionSmtp::COMMAND_AUTH_DIGESTMD5},
	{command_DIGESTMD5,     5 * 60,  5 * 60,  335, CExceptionSmtp::COMMAND_DIGESTMD5},
	{command_USER,          5 * 60,  5 * 60,  334, CExceptionSmtp::UNDEF_XYZ_RESPONSE},
	{command_PASSWORD,      5 * 60,  5 * 60,  235, CExceptionSmtp::BAD_LOGIN_PASS},
	{command_MAILFROM,      5 * 60,  5 * 60,  250, CExceptionSmtp::COMMAND_MAIL_FROM},
	{command_RCPTTO,        5 * 60,  5 * 60,  250, CExceptionSmtp::COMMAND_RCPT_TO},
	{command_DATA,          5 * 60,  2 * 60,  354, CExceptionSmtp::COMMAND_DATA},
	{command_DATABLOCK,     3 * 60,  0,     0,   CExceptionSmtp::COMMAND_DATABLOCK},	// Here the valid_reply_code is set to zero because there are no replies when sending data blocks
	{command_DATAEND,       3 * 60,  10 * 60, 250, CExceptionSmtp::MSG_BODY_ERROR},
	{command_QUIT,          5 * 60,  5 * 60,  221, CExceptionSmtp::COMMAND_QUIT},
	{command_STARTTLS,      5 * 60,  5 * 60,  220, CExceptionSmtp::COMMAND_EHLO_STARTTLS}
};

Command_Entry* FindCommandEntry(SMTP_COMMAND command)
{
	Command_Entry* pEntry = NULL;
	for (size_t i = 0; i < sizeof(command_list) / sizeof(command_list[0]); ++i)
	{
		if (command_list[i].command == command)
		{
			pEntry = &command_list[i];
			break;
		}
	}
	assert(pEntry != NULL);
	return pEntry;
}

// A simple string match
bool IsKeywordSupported(const char* response, const char* keyword)
{
	assert(response != NULL && keyword != NULL);
	if (response == NULL || keyword == NULL)
		return false;
	int res_len = strlen(response);
	int key_len = strlen(keyword);
	if (res_len < key_len)
		return false;
	int pos = 0;
	for (; pos < res_len - key_len + 1; ++pos)
	{
		if (_strnicmp(keyword, response + pos, key_len) == 0)
		{
			if (pos > 0 &&
				(response[pos - 1] == '-' ||
					response[pos - 1] == ' ' ||
					response[pos - 1] == '='))
			{
				if (pos + key_len < res_len)
				{
					if (response[pos + key_len] == ' ' ||
						response[pos + key_len] == '=')
					{
						return true;
					}
					else if (pos + key_len + 1 < res_len)
					{
						if (response[pos + key_len] == '\r' &&
							response[pos + key_len + 1] == '\n')
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

unsigned char* CharToUnsignedChar(const char *strIn)
{
	unsigned char *strOut;

	unsigned long length,
		i;


	length = strlen(strIn);

	strOut = new unsigned char[length + 1];
	if (!strOut) return NULL;

	for (i = 0; i < length; i++) strOut[i] = (unsigned char)strIn[i];
	strOut[length] = '\0';

	return strOut;
}

///@BRIEF	Gets the text description for a specified error code.
std::string CExceptionSmtp::GetErrorText() const
{
	switch (m_errorCode)
	{
	case CExceptionSmtp::CSMTP_NO_ERROR:
		return "";
	case CExceptionSmtp::WSA_STARTUP:
		return "Unable to initialise winsock2";
	case CExceptionSmtp::WSA_VER:
		return "Wrong version of the winsock2";
	case CExceptionSmtp::WSA_SEND:
		return "Function send() failed";
	case CExceptionSmtp::WSA_RECV:
		return "Function recv() failed";
	case CExceptionSmtp::WSA_CONNECT:
		return "Function connect failed";
	case CExceptionSmtp::WSA_GETHOSTBY_NAME_ADDR:
		return "Unable to determine remote server";
	case CExceptionSmtp::WSA_INVALID_SOCKET:
		return "Invalid winsock2 socket";
	case CExceptionSmtp::WSA_HOSTNAME:
		return "Function hostname() failed";
	case CExceptionSmtp::WSA_IOCTLSOCKET:
		return "Function ioctlsocket() failed";
	case CExceptionSmtp::BAD_IPV4_ADDR:
		return "Improper IPv4 address";
	case CExceptionSmtp::UNDEF_MSG_HEADER:
		return "Undefined message header";
	case CExceptionSmtp::UNDEF_MAIL_FROM:
		return "Undefined mail sender";
	case CExceptionSmtp::UNDEF_SUBJECT:
		return "Undefined message subject";
	case CExceptionSmtp::UNDEF_RECIPIENTS:
		return "Undefined at least one reciepent";
	case CExceptionSmtp::UNDEF_RECIPIENT_MAIL:
		return "Undefined recipent mail";
	case CExceptionSmtp::UNDEF_LOGIN:
		return "Undefined user login";
	case CExceptionSmtp::UNDEF_PASSWORD:
		return "Undefined user password";
	case CExceptionSmtp::BAD_LOGIN_PASSWORD:
		return "Invalid user login or password";
	case CExceptionSmtp::BAD_DIGEST_RESPONSE:
		return "Server returned a bad digest MD5 response";
	case CExceptionSmtp::BAD_SERVER_NAME:
		return "Unable to determine server name for digest MD5 response";
	case CExceptionSmtp::COMMAND_MAIL_FROM:
		return "Server returned error after sending MAIL FROM";
	case CExceptionSmtp::COMMAND_EHLO:
		return "Server returned error after sending EHLO";
	case CExceptionSmtp::COMMAND_AUTH_PLAIN:
		return "Server returned error after sending AUTH PLAIN";
	case CExceptionSmtp::COMMAND_AUTH_LOGIN:
		return "Server returned error after sending AUTH LOGIN";
	case CExceptionSmtp::COMMAND_AUTH_CRAMMD5:
		return "Server returned error after sending AUTH CRAM-MD5";
	case CExceptionSmtp::COMMAND_AUTH_DIGESTMD5:
		return "Server returned error after sending AUTH DIGEST-MD5";
	case CExceptionSmtp::COMMAND_DIGESTMD5:
		return "Server returned error after sending MD5 DIGEST";
	case CExceptionSmtp::COMMAND_DATA:
		return "Server returned error after sending DATA";
	case CExceptionSmtp::COMMAND_QUIT:
		return "Server returned error after sending QUIT";
	case CExceptionSmtp::COMMAND_RCPT_TO:
		return "Server returned error after sending RCPT TO";
	case CExceptionSmtp::MSG_BODY_ERROR:
		return "Error in message body";
	case CExceptionSmtp::CONNECTION_CLOSED:
		return "Server has closed the connection";
	case CExceptionSmtp::SERVER_NOT_READY:
		return "Server is not ready";
	case CExceptionSmtp::SERVER_NOT_RESPONDING:
		return "Server not responding";
	case CExceptionSmtp::FILE_NOT_EXIST:
		return "Attachment file does not exist";
	case CExceptionSmtp::MSG_TOO_BIG:
		return "Message is too big";
	case CExceptionSmtp::BAD_LOGIN_PASS:
		return "Bad login or password";
	case CExceptionSmtp::UNDEF_XYZ_RESPONSE:
		return "Undefined xyz SMTP response";
	case CExceptionSmtp::LACK_OF_MEMORY:
		return "Lack of memory";
	case CExceptionSmtp::TIME_ERROR:
		return "time() error";
	case CExceptionSmtp::RECVBUF_IS_EMPTY:
		return "RecvBuf is empty";
	case CExceptionSmtp::SENDBUF_IS_EMPTY:
		return "SendBuf is empty";
	case CExceptionSmtp::OUT_OF_MSG_RANGE:
		return "Specified line number is out of message size";
	case CExceptionSmtp::COMMAND_EHLO_STARTTLS:
		return "Server returned error after sending STARTTLS";
	case CExceptionSmtp::SSL_PROBLEM:
		return "SSL problem";
	case CExceptionSmtp::COMMAND_DATABLOCK:
		return "Failed to send data block";
	case CExceptionSmtp::STARTTLS_NOT_SUPPORTED:
		return "The STARTTLS command is not supported by the server";
	case CExceptionSmtp::LOGIN_NOT_SUPPORTED:
		return "AUTH LOGIN is not supported by the server";
	default:
		return "Undefined error id";
	}
}

///@BRIEF	CPostman constructor.
CPostman::CPostman()
{
	m_smtpSocket = INVALID_SOCKET;
	m_bConnected = false;
	m_iXPriority = XPRIORITY_NORMAL;
	m_iSMTPSrvPort = 0;
	m_bAuthenticate = true;

#ifndef LINUX
	// Initialize WinSock
	WSADATA wsaData;
	WORD wVer = MAKEWORD(2, 2);
	if (WSAStartup(wVer, &wsaData) != NO_ERROR)
		throw CExceptionSmtp(CExceptionSmtp::WSA_STARTUP);
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		WSACleanup();
		throw CExceptionSmtp(CExceptionSmtp::WSA_VER);
	}
#endif

	char hostname[255];
	if (gethostname((char *)&hostname, 255) == SOCKET_ERROR) 
		throw CExceptionSmtp(CExceptionSmtp::WSA_HOSTNAME);
	m_sLocalHostName = hostname;

	if ((RecvBuf = new char[BUFFER_SIZE]) == NULL)
		throw CExceptionSmtp(CExceptionSmtp::LACK_OF_MEMORY);

	if ((SendBuf = new char[BUFFER_SIZE]) == NULL)
		throw CExceptionSmtp(CExceptionSmtp::LACK_OF_MEMORY);

	m_type = NO_SECURITY;
	m_ctx = NULL;
	m_ssl = NULL;
	m_bHTML = false;
	m_bReadReceipt = false;

	m_sCharSet = "US-ASCII";
}

///@BRIEF	CPostman destructor.
CPostman::~CPostman()
{
	if (m_bConnected) 
		DisconnectRemoteServer();

	if (SendBuf)
	{
		delete[] SendBuf;
		SendBuf = NULL;
	}
	if (RecvBuf)
	{
		delete[] RecvBuf;
		RecvBuf = NULL;
	}

	CleanupOpenSSL();

#ifndef LINUX
	WSACleanup();
#endif
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: AddAttachment
// DESCRIPTION: New attachment is added.
//   ARGUMENTS: const char *Path - name of attachment added
// USES GLOBAL: Attachments
// MODIFIES GL: Attachments
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::AddAttachment(const char *Path)
{
	assert(Path);
	Attachments.insert(Attachments.end(), Path);
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: AddRecipient
// DESCRIPTION: New recipient data is added i.e.: email and name. .
//   ARGUMENTS: const char *email - mail of the recipient
//              const char *name - name of the recipient
// USES GLOBAL: Recipients
// MODIFIES GL: Recipients
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::AddRecipient(const char *email, const char *name)
{
	if (!email)
		throw CExceptionSmtp(CExceptionSmtp::UNDEF_RECIPIENT_MAIL);

	Recipient recipient;
	recipient.Mail = email;
	if (name != NULL) recipient.Name = name;
	else recipient.Name.empty();

	Recipients.insert(Recipients.end(), recipient);
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: AddCCRecipient
// DESCRIPTION: New cc-recipient data is added i.e.: email and name. .
//   ARGUMENTS: const char *email - mail of the cc-recipient
//              const char *name - name of the ccc-recipient
// USES GLOBAL: CCRecipients
// MODIFIES GL: CCRecipients
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::AddCCRecipient(const char *email, const char *name)
{
	if (!email)
		throw CExceptionSmtp(CExceptionSmtp::UNDEF_RECIPIENT_MAIL);

	Recipient recipient;
	recipient.Mail = email;
	if (name != NULL) recipient.Name = name;
	else recipient.Name.empty();

	CCRecipients.insert(CCRecipients.end(), recipient);
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: AddBCCRecipient
// DESCRIPTION: New bcc-recipient data is added i.e.: email and name. .
//   ARGUMENTS: const char *email - mail of the bcc-recipient
//              const char *name - name of the bccc-recipient
// USES GLOBAL: BCCRecipients
// MODIFIES GL: BCCRecipients
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::AddBCCRecipient(const char *email, const char *name)
{
	if (!email)
		throw CExceptionSmtp(CExceptionSmtp::UNDEF_RECIPIENT_MAIL);

	Recipient recipient;
	recipient.Mail = email;
	if (name != NULL) recipient.Name = name;
	else recipient.Name.empty();

	BCCRecipients.insert(BCCRecipients.end(), recipient);
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: AddMsgLine
// DESCRIPTION: Adds new line in a message.
//   ARGUMENTS: const char *Text - text of the new line
// USES GLOBAL: MsgBody
// MODIFIES GL: MsgBody
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::AddMsgLine(const char* Text)
{
	MsgBody.insert(MsgBody.end(), Text);
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: DelMsgLine
// DESCRIPTION: Deletes specified line in text message.. .
//   ARGUMENTS: unsigned int Line - line to be delete
// USES GLOBAL: MsgBody
// MODIFIES GL: MsgBody
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::DelMsgLine(unsigned int Line)
{
	if (Line >= MsgBody.size())
		throw CExceptionSmtp(CExceptionSmtp::OUT_OF_MSG_RANGE);
	MsgBody.erase(MsgBody.begin() + Line);
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: DelRecipients
// DESCRIPTION: Deletes all recipients. .
//   ARGUMENTS: void
// USES GLOBAL: Recipients
// MODIFIES GL: Recipients
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::DelRecipients()
{
	Recipients.clear();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: DelBCCRecipients
// DESCRIPTION: Deletes all BCC recipients. .
//   ARGUMENTS: void
// USES GLOBAL: BCCRecipients
// MODIFIES GL: BCCRecipients
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::DelBCCRecipients()
{
	BCCRecipients.clear();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: DelCCRecipients
// DESCRIPTION: Deletes all CC recipients. .
//   ARGUMENTS: void
// USES GLOBAL: CCRecipients
// MODIFIES GL: CCRecipients
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::DelCCRecipients()
{
	CCRecipients.clear();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: DelMsgLines
// DESCRIPTION: Deletes message text.
//   ARGUMENTS: void
// USES GLOBAL: MsgBody
// MODIFIES GL: MsgBody
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::DelMsgLines()
{
	MsgBody.clear();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: DelAttachments
// DESCRIPTION: Deletes all recipients. .
//   ARGUMENTS: void
// USES GLOBAL: Attchments
// MODIFIES GL: Attachments
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::DelAttachments()
{
	Attachments.clear();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: ModMsgLine
// DESCRIPTION: Modifies a specific line of the message body
//   ARGUMENTS: unsigned int Line - the line number to modify
//              const char* Text - the new contents of the line
// USES GLOBAL: MsgBody
// MODIFIES GL: MsgBody
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::ModMsgLine(unsigned int Line, const char* Text)
{
	if (Text)
	{
		if (Line >= MsgBody.size())
			throw CExceptionSmtp(CExceptionSmtp::OUT_OF_MSG_RANGE);
		MsgBody.at(Line) = std::string(Text);
	}
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: ClearMessage
// DESCRIPTION: Clears the recipients and message body
//   ARGUMENTS: none
//     RETURNS: none
//      AUTHOR: David Johns
// AUTHOR/DATE: DRJ 2013-05-20
////////////////////////////////////////////////////////////////////////////////
void CPostman::ClearMessage()
{
	DelRecipients();
	DelBCCRecipients();
	DelCCRecipients();
	DelAttachments();
	DelMsgLines();
}

///@BRIEF	Sends out an email.
void CPostman::Send()
{
	unsigned int i, rcpt_count, res, FileId;
	char *FileBuf = NULL;
	FILE* hFile = NULL;
	unsigned long int FileSize, TotalSize, MsgPart;
	string FileName, EncodedFileName;
	string::size_type pos;

	///[1] Connecting to the SMTP server host if it is not yet connected.	
	if (m_smtpSocket == INVALID_SOCKET)
	{
		if (!ConnectToSmtpServer(m_sSMTPSrvName.c_str(), m_iSMTPSrvPort, m_type, m_bAuthenticate))		
			throw CExceptionSmtp(CExceptionSmtp::WSA_INVALID_SOCKET);
	}

	try {
		//Allocate memory
		if ((FileBuf = new char[55]) == NULL)
			throw CExceptionSmtp(CExceptionSmtp::LACK_OF_MEMORY);

		//Check that any attachments specified can be opened
		TotalSize = 0;
		for (FileId = 0; FileId < Attachments.size(); FileId++)
		{
			// opening the file:
			hFile = fopen(Attachments[FileId].c_str(), "rb");
			if (hFile == NULL)
				throw CExceptionSmtp(CExceptionSmtp::FILE_NOT_EXIST);

			// checking file size:
			fseek(hFile, 0, SEEK_END);
			FileSize = ftell(hFile);
			TotalSize += FileSize;

			// sending the file:
			if (TotalSize / 1024 > MSG_SIZE_IN_MB * 1024)
				throw CExceptionSmtp(CExceptionSmtp::MSG_TOO_BIG);

			fclose(hFile);
			hFile = NULL;
		}

		// ***** SENDING E-MAIL *****

		// MAIL <SP> FROM:<reverse-path> <CRLF>
		if (!m_sMailFrom.size())
			throw CExceptionSmtp(CExceptionSmtp::UNDEF_MAIL_FROM);
		Command_Entry* pEntry = FindCommandEntry(command_MAILFROM);
		snprintf(SendBuf, BUFFER_SIZE, "MAIL FROM:<%s>\r\n", m_sMailFrom.c_str());
		SendData(pEntry);
		ReceiveResponse(pEntry);

		// RCPT <SP> TO:<forward-path> <CRLF>
		if (!(rcpt_count = Recipients.size()))
			throw CExceptionSmtp(CExceptionSmtp::UNDEF_RECIPIENTS);
		pEntry = FindCommandEntry(command_RCPTTO);
		for (i = 0; i < Recipients.size(); i++)
		{
			snprintf(SendBuf, BUFFER_SIZE, "RCPT TO:<%s>\r\n", (Recipients.at(i).Mail).c_str());
			SendData(pEntry);
			ReceiveResponse(pEntry);
		}

		for (i = 0; i < CCRecipients.size(); i++)
		{
			snprintf(SendBuf, BUFFER_SIZE, "RCPT TO:<%s>\r\n", (CCRecipients.at(i).Mail).c_str());
			SendData(pEntry);
			ReceiveResponse(pEntry);
		}

		for (i = 0; i < BCCRecipients.size(); i++)
		{
			snprintf(SendBuf, BUFFER_SIZE, "RCPT TO:<%s>\r\n", (BCCRecipients.at(i).Mail).c_str());
			SendData(pEntry);
			ReceiveResponse(pEntry);
		}

		pEntry = FindCommandEntry(command_DATA);
		// DATA <CRLF>
		snprintf(SendBuf, BUFFER_SIZE, "DATA\r\n");
		SendData(pEntry);
		ReceiveResponse(pEntry);

		pEntry = FindCommandEntry(command_DATABLOCK);
		// send header(s)
		FormatHeader(SendBuf);
		SendData(pEntry);

		// send text message
		if (GetMsgLines())
		{
			for (i = 0; i < GetMsgLines(); i++)
			{
				snprintf(SendBuf, BUFFER_SIZE, "%s\r\n", GetMsgLineText(i));
				SendData(pEntry);
			}
		}
		else
		{
			snprintf(SendBuf, BUFFER_SIZE, "%s\r\n", " ");
			SendData(pEntry);
		}

		// next goes attachments (if they are)
		for (FileId = 0; FileId < Attachments.size(); FileId++)
		{
#ifndef LINUX
			pos = Attachments[FileId].find_last_of("\\");
#else
			pos = Attachments[FileId].find_last_of("/");
#endif
			if (pos == string::npos) FileName = Attachments[FileId];
			else FileName = Attachments[FileId].substr(pos + 1);

			//RFC 2047 - Use UTF-8 charset,base64 encode.
			EncodedFileName = "=?UTF-8?B?";
			EncodedFileName += base64_encode((unsigned char *)FileName.c_str(), FileName.size());
			EncodedFileName += "?=";

			snprintf(SendBuf, BUFFER_SIZE, "--%s\r\n", BOUNDARY_TEXT);
			strcat(SendBuf, "Content-Type: application/x-msdownload; name=\"");
			strcat(SendBuf, EncodedFileName.c_str());
			strcat(SendBuf, "\"\r\n");
			strcat(SendBuf, "Content-Transfer-Encoding: base64\r\n");
			strcat(SendBuf, "Content-Disposition: attachment; filename=\"");
			strcat(SendBuf, EncodedFileName.c_str());
			strcat(SendBuf, "\"\r\n");
			strcat(SendBuf, "\r\n");

			SendData(pEntry);

			// opening the file:
			hFile = fopen(Attachments[FileId].c_str(), "rb");
			if (hFile == NULL)
				throw CExceptionSmtp(CExceptionSmtp::FILE_NOT_EXIST);

			// get file size:
			fseek(hFile, 0, SEEK_END);
			FileSize = ftell(hFile);
			fseek(hFile, 0, SEEK_SET);

			MsgPart = 0;
			for (i = 0; i < FileSize / 54 + 1; i++)
			{
				res = fread(FileBuf, sizeof(char), 54, hFile);
				MsgPart ? strcat(SendBuf, base64_encode(reinterpret_cast<const unsigned char*>(FileBuf), res).c_str())
					: strcpy(SendBuf, base64_encode(reinterpret_cast<const unsigned char*>(FileBuf), res).c_str());
				strcat(SendBuf, "\r\n");
				MsgPart += res + 2;
				if (MsgPart >= BUFFER_SIZE / 2)
				{ // sending part of the message
					MsgPart = 0;
					SendData(pEntry); // FileBuf, FileName, fclose(hFile);
				}
			}
			if (MsgPart)
			{
				SendData(pEntry); // FileBuf, FileName, fclose(hFile);
			}
			fclose(hFile);
			hFile = NULL;
		}
		delete[] FileBuf;
		FileBuf = NULL;

		// sending last message block (if there is one or more attachments)
		if (Attachments.size())
		{
			snprintf(SendBuf, BUFFER_SIZE, "\r\n--%s--\r\n", BOUNDARY_TEXT);
			SendData(pEntry);
		}

		pEntry = FindCommandEntry(command_DATAEND);
		// <CRLF> . <CRLF>
		snprintf(SendBuf, BUFFER_SIZE, "\r\n.\r\n");
		SendData(pEntry);
		ReceiveResponse(pEntry);
	}
	catch (const CExceptionSmtp&)
	{
		if (hFile) fclose(hFile);
		if (FileBuf) delete[] FileBuf;
		DisconnectRemoteServer();
		throw;
	}
}

/*
** @BRIEF	Connects to the SMTP server host.
** @PARAM	szServer - the domain or IP address of the SMTP server host.
** @PARAM	smtpPortNum_ - the port number of the SMTP server.
*/
bool CPostman::ConnectToSmtpServer(
	const char* szServer,
	const unsigned short smtpPortNum_,
	SMTP_SECURITY_TYPE securityType_,
	bool authenticate,
	const char* login,
	const char* password)
{
	unsigned short _smtpPortNum = 0;
	LPSERVENT lpServEnt;
	SOCKADDR_IN sockAddr;
	unsigned long ul = 1;
	fd_set fdwrite, fdexcept;
	timeval timeout;
	int res = 0;

	try
	{
		///[1] Connect to the SMTP server.
		timeout.tv_sec = TIME_IN_SEC; //3 minutes
		timeout.tv_usec = 0;

		//Fetch the SMTP host port number
		if (smtpPortNum_ != 0)
			_smtpPortNum = htons(smtpPortNum_);
		else
		{
			lpServEnt = getservbyname("mail", 0);
			if (lpServEnt == nullptr)
				_smtpPortNum = htons(25); //The default port number for SMTP.
			else
				_smtpPortNum = lpServEnt->s_port;
		}

		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = _smtpPortNum;
		//Fetch the SMTP host address
		if ((sockAddr.sin_addr.s_addr = inet_addr(szServer)) == INADDR_NONE)
		{
			LPHOSTENT host;

			host = gethostbyname(szServer);
			if (host)
				memcpy(&sockAddr.sin_addr, host->h_addr_list[0], host->h_length);
			else
			{
#ifdef LINUX
				close(m_smtpSocket);
#else
				closesocket(m_smtpSocket);
#endif
				throw CExceptionSmtp(CExceptionSmtp::WSA_GETHOSTBY_NAME_ADDR);
			}
		}
		
		m_smtpSocket = INVALID_SOCKET;
		if ((m_smtpSocket = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
			throw CExceptionSmtp(CExceptionSmtp::WSA_INVALID_SOCKET);
		//Sets the socket to the non-blocking mode
#ifdef LINUX
		if (ioctl(m_smtpSocket, FIONBIO, (unsigned long*)&ul) == SOCKET_ERROR)
#else
		if (ioctlsocket(m_smtpSocket, FIONBIO, (unsigned long*)&ul) == SOCKET_ERROR) 
#endif
		{
#ifdef LINUX
			close(m_smtpSocket);
#else
			closesocket(m_smtpSocket);
#endif
			throw CExceptionSmtp(CExceptionSmtp::WSA_IOCTLSOCKET);
		}

		if (connect(m_smtpSocket, (LPSOCKADDR)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR)
		{
#ifdef LINUX
			if (errno != EINPROGRESS)
#else
			if (WSAGetLastError() != WSAEWOULDBLOCK)
#endif
			{
#ifdef LINUX
				close(m_smtpSocket);
#else
				closesocket(m_smtpSocket);
#endif
				throw CExceptionSmtp(CExceptionSmtp::WSA_CONNECT);
			}
		}
		else
			return true;

		while (true)
		{
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdexcept);

			FD_SET(m_smtpSocket, &fdwrite);
			FD_SET(m_smtpSocket, &fdexcept);

			if ((res = select(m_smtpSocket + 1, NULL, &fdwrite, &fdexcept, &timeout)) == SOCKET_ERROR)
			{
#ifdef LINUX
				close(m_smtpSocket);
#else
				closesocket(m_smtpSocket);
#endif
				throw CExceptionSmtp(CExceptionSmtp::WSA_SELECT);
			}

			if (!res) //Select() times out.
			{
#ifdef LINUX
				close(m_smtpSocket);
#else
				closesocket(m_smtpSocket);
#endif
				throw CExceptionSmtp(CExceptionSmtp::SELECT_TIMEOUT);
			}
			if (res && FD_ISSET(m_smtpSocket, &fdwrite))
				break;
			if (res && FD_ISSET(m_smtpSocket, &fdexcept))
			{
#ifdef LINUX
				close(m_smtpSocket);
#else
				closesocket(m_smtpSocket);
#endif
				throw CExceptionSmtp(CExceptionSmtp::WSA_SELECT);
			}
		} // while

		FD_CLR(m_smtpSocket, &fdwrite);
		FD_CLR(m_smtpSocket, &fdexcept);


		///[2] Initialisation on SSL or TLS if any
		if (securityType_ != DO_NOT_SET) 
			SetSecurityType(securityType_);
		if (GetSecurityType() == USE_TLS || GetSecurityType() == USE_SSL)
		{
			InitOpenSSL();
			if (GetSecurityType() == USE_SSL)
			{
				OpenSSLConnect();
			}
		}

		Command_Entry* pEntry = FindCommandEntry(command_INIT);
		ReceiveResponse(pEntry);

		SayHello();

		if (GetSecurityType() == USE_TLS)
		{
			StartTls();
			SayHello();
		}

		if (authenticate && IsKeywordSupported(RecvBuf, "AUTH") == true)
		{
			if (login) SetLogin(login);
			if (!m_sLogin.size())
				throw CExceptionSmtp(CExceptionSmtp::UNDEF_LOGIN);

			if (password) SetPassword(password);
			if (!m_sPassword.size())
				throw CExceptionSmtp(CExceptionSmtp::UNDEF_PASSWORD);

			if (IsKeywordSupported(RecvBuf, "LOGIN") == true)
			{
				pEntry = FindCommandEntry(command_AUTHLOGIN);
				snprintf(SendBuf, BUFFER_SIZE, "AUTH LOGIN\r\n");
				SendData(pEntry);
				ReceiveResponse(pEntry);

				// send login:
				std::string encoded_login = base64_encode(reinterpret_cast<const unsigned char*>(m_sLogin.c_str()), m_sLogin.size());
				pEntry = FindCommandEntry(command_USER);
				snprintf(SendBuf, BUFFER_SIZE, "%s\r\n", encoded_login.c_str());
				SendData(pEntry);
				ReceiveResponse(pEntry);

				// send password:
				std::string encoded_password = base64_encode(reinterpret_cast<const unsigned char*>(m_sPassword.c_str()), m_sPassword.size());
				pEntry = FindCommandEntry(command_PASSWORD);
				snprintf(SendBuf, BUFFER_SIZE, "%s\r\n", encoded_password.c_str());
				SendData(pEntry);
				ReceiveResponse(pEntry);
			}
			else if (IsKeywordSupported(RecvBuf, "PLAIN") == true)
			{
				pEntry = FindCommandEntry(command_AUTHPLAIN);
				snprintf(SendBuf, BUFFER_SIZE, "%s^%s^%s", m_sLogin.c_str(), m_sLogin.c_str(), m_sPassword.c_str());
				unsigned int length = strlen(SendBuf);
				unsigned char *ustrLogin = CharToUnsignedChar(SendBuf);
				for (unsigned int i = 0; i < length; i++)
				{
					if (ustrLogin[i] == 94) ustrLogin[i] = 0;
				}
				std::string encoded_login = base64_encode(ustrLogin, length);
				delete[] ustrLogin;
				snprintf(SendBuf, BUFFER_SIZE, "AUTH PLAIN %s\r\n", encoded_login.c_str());
				SendData(pEntry);
				ReceiveResponse(pEntry);
			}
			else if (IsKeywordSupported(RecvBuf, "CRAM-MD5") == true)
			{
				pEntry = FindCommandEntry(command_AUTHCRAMMD5);
				snprintf(SendBuf, BUFFER_SIZE, "AUTH CRAM-MD5\r\n");
				SendData(pEntry);
				ReceiveResponse(pEntry);

				std::string encoded_challenge = RecvBuf;
				encoded_challenge = encoded_challenge.substr(4);
				std::string decoded_challenge = base64_decode(encoded_challenge);

				/////////////////////////////////////////////////////////////////////
				//test data from RFC 2195
				//decoded_challenge = "<1896.697170952@postoffice.reston.mci.net>";
				//m_sLogin = "tim";
				//m_sPassword = "tanstaaftanstaaf";
				//MD5 should produce b913a602c7eda7a495b4e6e7334d3890
				//should encode as dGltIGI5MTNhNjAyYzdlZGE3YTQ5NWI0ZTZlNzMzNGQzODkw
				/////////////////////////////////////////////////////////////////////

				unsigned char *ustrChallenge = CharToUnsignedChar(decoded_challenge.c_str());
				unsigned char *ustrPassword = CharToUnsignedChar(m_sPassword.c_str());
				if (!ustrChallenge || !ustrPassword)
					throw CExceptionSmtp(CExceptionSmtp::BAD_LOGIN_PASSWORD);

				// if ustrPassword is longer than 64 bytes reset it to ustrPassword=MD5(ustrPassword)
				int passwordLength = m_sPassword.size();
				if (passwordLength > 64) {
					MD5 md5password;
					md5password.update(ustrPassword, passwordLength);
					md5password.finalize();
					ustrPassword = md5password.raw_digest();
					passwordLength = 16;
				}

				//Storing ustrPassword in pads
				unsigned char ipad[65], opad[65];
				memset(ipad, 0, 64);
				memset(opad, 0, 64);
				memcpy(ipad, ustrPassword, passwordLength);
				memcpy(opad, ustrPassword, passwordLength);

				// XOR ustrPassword with ipad and opad values
				for (int i = 0; i < 64; i++) {
					ipad[i] ^= 0x36;
					opad[i] ^= 0x5c;
				}

				//perform inner MD5
				MD5 md5pass1;
				md5pass1.update(ipad, 64);
				md5pass1.update(ustrChallenge, decoded_challenge.size());
				md5pass1.finalize();
				unsigned char *ustrResult = md5pass1.raw_digest();

				//perform outer MD5
				MD5 md5pass2;
				md5pass2.update(opad, 64);
				md5pass2.update(ustrResult, 16);
				md5pass2.finalize();
				decoded_challenge = md5pass2.hex_digest();

				delete[] ustrChallenge;
				delete[] ustrPassword;
				delete[] ustrResult;

				decoded_challenge = m_sLogin + " " + decoded_challenge;
				encoded_challenge = base64_encode(reinterpret_cast<const unsigned char*>(decoded_challenge.c_str()), decoded_challenge.size());

				snprintf(SendBuf, BUFFER_SIZE, "%s\r\n", encoded_challenge.c_str());
				pEntry = FindCommandEntry(command_PASSWORD);
				SendData(pEntry);
				ReceiveResponse(pEntry);
			}
			else if (IsKeywordSupported(RecvBuf, "DIGEST-MD5") == true)
			{
				pEntry = FindCommandEntry(command_DIGESTMD5);
				snprintf(SendBuf, BUFFER_SIZE, "AUTH DIGEST-MD5\r\n");
				SendData(pEntry);
				ReceiveResponse(pEntry);

				std::string encoded_challenge = RecvBuf;
				encoded_challenge = encoded_challenge.substr(4);
				std::string decoded_challenge = base64_decode(encoded_challenge);

				/////////////////////////////////////////////////////////////////////
				//Test data from RFC 2831
				//To test jump into authenticate and read this line and the ones down to next test data section
				//decoded_challenge = "realm=\"elwood.innosoft.com\",nonce=\"OA6MG9tEQGm2hh\",qop=\"auth\",algorithm=md5-sess,charset=utf-8";
				/////////////////////////////////////////////////////////////////////

				//Get the nonce (manditory)
				int find = decoded_challenge.find("nonce");
				if (find < 0)
					throw CExceptionSmtp(CExceptionSmtp::BAD_DIGEST_RESPONSE);
				std::string nonce = decoded_challenge.substr(find + 7);
				find = nonce.find("\"");
				if (find < 0)
					throw CExceptionSmtp(CExceptionSmtp::BAD_DIGEST_RESPONSE);
				nonce = nonce.substr(0, find);

				//Get the realm (optional)
				std::string realm;
				find = decoded_challenge.find("realm");
				if (find >= 0) {
					realm = decoded_challenge.substr(find + 7);
					find = realm.find("\"");
					if (find < 0)
						throw CExceptionSmtp(CExceptionSmtp::BAD_DIGEST_RESPONSE);
					realm = realm.substr(0, find);
				}

				//Create a cnonce
				char cnonce[17], nc[9];
				snprintf(cnonce, 17, "%x", (unsigned int)time(NULL));

				//Set nonce count
				snprintf(nc, 9, "%08d", 1);

				//Set QOP
				std::string qop = "auth";

				//Get server address and set uri
				//Skip this step during test
#ifdef __linux__
				socklen_t len;
#else
				int len;
#endif
				struct sockaddr_storage addr;
				len = sizeof addr;
				if (!getpeername(m_smtpSocket, (struct sockaddr*)&addr, &len))
					throw CExceptionSmtp(CExceptionSmtp::BAD_SERVER_NAME);

				struct sockaddr_in *s = (struct sockaddr_in *)&addr;
				std::string uri = inet_ntoa(s->sin_addr);
				uri = "smtp/" + uri;

				/////////////////////////////////////////////////////////////////////
				//test data from RFC 2831
				//m_sLogin = "chris";
				//m_sPassword = "secret";
				//snprintf(cnonce, 17, "OA6MHXh6VqTrRk");
				//uri = "imap/elwood.innosoft.com";
				//Should form the response:
				//    charset=utf-8,username="chris",
				//    realm="elwood.innosoft.com",nonce="OA6MG9tEQGm2hh",nc=00000001,
				//    cnonce="OA6MHXh6VqTrRk",digest-uri="imap/elwood.innosoft.com",
				//    response=d388dad90d4bbd760a152321f2143af7,qop=auth
				//This encodes to:
				//    Y2hhcnNldD11dGYtOCx1c2VybmFtZT0iY2hyaXMiLHJlYWxtPSJlbHdvb2
				//    QuaW5ub3NvZnQuY29tIixub25jZT0iT0E2TUc5dEVRR20yaGgiLG5jPTAw
				//    MDAwMDAxLGNub25jZT0iT0E2TUhYaDZWcVRyUmsiLGRpZ2VzdC11cmk9Im
				//    ltYXAvZWx3b29kLmlubm9zb2Z0LmNvbSIscmVzcG9uc2U9ZDM4OGRhZDkw
				//    ZDRiYmQ3NjBhMTUyMzIxZjIxNDNhZjcscW9wPWF1dGg=
				/////////////////////////////////////////////////////////////////////

				//Calculate digest response
				unsigned char *ustrRealm = CharToUnsignedChar(realm.c_str());
				unsigned char *ustrUsername = CharToUnsignedChar(m_sLogin.c_str());
				unsigned char *ustrPassword = CharToUnsignedChar(m_sPassword.c_str());
				unsigned char *ustrNonce = CharToUnsignedChar(nonce.c_str());
				unsigned char *ustrCNonce = CharToUnsignedChar(cnonce);
				unsigned char *ustrUri = CharToUnsignedChar(uri.c_str());
				unsigned char *ustrNc = CharToUnsignedChar(nc);
				unsigned char *ustrQop = CharToUnsignedChar(qop.c_str());
				if (!ustrRealm || !ustrUsername || !ustrPassword || !ustrNonce || !ustrCNonce || !ustrUri || !ustrNc || !ustrQop)
					throw CExceptionSmtp(CExceptionSmtp::BAD_LOGIN_PASSWORD);

				MD5 md5a1a;
				md5a1a.update(ustrUsername, m_sLogin.size());
				md5a1a.update((unsigned char*)":", 1);
				md5a1a.update(ustrRealm, realm.size());
				md5a1a.update((unsigned char*)":", 1);
				md5a1a.update(ustrPassword, m_sPassword.size());
				md5a1a.finalize();
				unsigned char *ua1 = md5a1a.raw_digest();

				MD5 md5a1b;
				md5a1b.update(ua1, 16);
				md5a1b.update((unsigned char*)":", 1);
				md5a1b.update(ustrNonce, nonce.size());
				md5a1b.update((unsigned char*)":", 1);
				md5a1b.update(ustrCNonce, strlen(cnonce));
				//authzid could be added here
				md5a1b.finalize();
				char *a1 = md5a1b.hex_digest();

				MD5 md5a2;
				md5a2.update((unsigned char*) "AUTHENTICATE:", 13);
				md5a2.update(ustrUri, uri.size());
				//authint and authconf add an additional line here	
				md5a2.finalize();
				char *a2 = md5a2.hex_digest();

				delete[] ua1;
				ua1 = CharToUnsignedChar(a1);
				unsigned char *ua2 = CharToUnsignedChar(a2);

				//compute KD
				MD5 md5;
				md5.update(ua1, 32);
				md5.update((unsigned char*)":", 1);
				md5.update(ustrNonce, nonce.size());
				md5.update((unsigned char*)":", 1);
				md5.update(ustrNc, strlen(nc));
				md5.update((unsigned char*)":", 1);
				md5.update(ustrCNonce, strlen(cnonce));
				md5.update((unsigned char*)":", 1);
				md5.update(ustrQop, qop.size());
				md5.update((unsigned char*)":", 1);
				md5.update(ua2, 32);
				md5.finalize();
				decoded_challenge = md5.hex_digest();

				delete[] ustrRealm;
				delete[] ustrUsername;
				delete[] ustrPassword;
				delete[] ustrNonce;
				delete[] ustrCNonce;
				delete[] ustrUri;
				delete[] ustrNc;
				delete[] ustrQop;
				delete[] ua1;
				delete[] ua2;
				delete[] a1;
				delete[] a2;

				//send the response
				if (strstr(RecvBuf, "charset") >= 0) snprintf(SendBuf, BUFFER_SIZE, "charset=utf-8,username=\"%s\"", m_sLogin.c_str());
				else snprintf(SendBuf, BUFFER_SIZE, "username=\"%s\"", m_sLogin.c_str());
				if (!realm.empty()) {
					snprintf(RecvBuf, BUFFER_SIZE, ",realm=\"%s\"", realm.c_str());
					strcat(SendBuf, RecvBuf);
				}
				snprintf(RecvBuf, BUFFER_SIZE, ",nonce=\"%s\"", nonce.c_str());
				strcat(SendBuf, RecvBuf);
				snprintf(RecvBuf, BUFFER_SIZE, ",nc=%s", nc);
				strcat(SendBuf, RecvBuf);
				snprintf(RecvBuf, BUFFER_SIZE, ",cnonce=\"%s\"", cnonce);
				strcat(SendBuf, RecvBuf);
				snprintf(RecvBuf, BUFFER_SIZE, ",digest-uri=\"%s\"", uri.c_str());
				strcat(SendBuf, RecvBuf);
				snprintf(RecvBuf, BUFFER_SIZE, ",response=%s", decoded_challenge.c_str());
				strcat(SendBuf, RecvBuf);
				snprintf(RecvBuf, BUFFER_SIZE, ",qop=%s", qop.c_str());
				strcat(SendBuf, RecvBuf);
				unsigned char *ustrDigest = CharToUnsignedChar(SendBuf);
				encoded_challenge = base64_encode(ustrDigest, strlen(SendBuf));
				delete[] ustrDigest;
				snprintf(SendBuf, BUFFER_SIZE, "%s\r\n", encoded_challenge.c_str());
				pEntry = FindCommandEntry(command_DIGESTMD5);
				SendData(pEntry);
				ReceiveResponse(pEntry);

				//Send completion carraige return
				snprintf(SendBuf, BUFFER_SIZE, "\r\n");
				pEntry = FindCommandEntry(command_PASSWORD);
				SendData(pEntry);
				ReceiveResponse(pEntry);
			}
			else throw CExceptionSmtp(CExceptionSmtp::LOGIN_NOT_SUPPORTED);
		}
	}
	catch (const CExceptionSmtp&)
	{
		if (RecvBuf[0] == '5' && RecvBuf[1] == '3' && RecvBuf[2] == '0')
			m_bConnected = false;
		DisconnectRemoteServer();
		throw;
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: DisconnectRemoteServer
// DESCRIPTION: Disconnects from the SMTP server and closes the socket
//   ARGUMENTS: none
// USES GLOBAL: none
// MODIFIES GL: none
//     RETURNS: void
//      AUTHOR: David Johns
// AUTHOR/DATE: DRJ 2010-08-14
////////////////////////////////////////////////////////////////////////////////
void CPostman::DisconnectRemoteServer()
{
	if (m_bConnected) SayQuit();
	if (m_smtpSocket)
	{
#ifdef LINUX
		close(m_smtpSocket);
#else
		closesocket(m_smtpSocket);
#endif
	}
	m_smtpSocket = INVALID_SOCKET;
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: SmtpXYZdigits
// DESCRIPTION: Converts three letters from RecvBuf to the number.
//   ARGUMENTS: none
// USES GLOBAL: RecvBuf
// MODIFIES GL: none
//     RETURNS: integer number
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
int CPostman::SmtpXYZdigits()
{
	assert(RecvBuf);
	if (RecvBuf == NULL)
		return 0;
	return (RecvBuf[0] - '0') * 100 + (RecvBuf[1] - '0') * 10 + RecvBuf[2] - '0';
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: FormatHeader
// DESCRIPTION: Prepares a header of the message.
//   ARGUMENTS: char* header - formated header string
// USES GLOBAL: Recipients, CCRecipients, BCCRecipients
// MODIFIES GL: none
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
void CPostman::FormatHeader(char* header)
{
	char month[][4] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };
	size_t i;
	std::string to;
	std::string cc;
	std::string bcc;
	time_t rawtime;
	struct tm* timeinfo;

	// date/time check
	if (time(&rawtime) > 0)
		timeinfo = localtime(&rawtime);
	else
		throw CExceptionSmtp(CExceptionSmtp::TIME_ERROR);

	// check for at least one recipient
	if (Recipients.size())
	{
		for (i = 0; i < Recipients.size(); i++)
		{
			if (i > 0)
				to.append(",");
			to += Recipients[i].Name;
			to.append("<");
			to += Recipients[i].Mail;
			to.append(">");
		}
	}
	else
		throw CExceptionSmtp(CExceptionSmtp::UNDEF_RECIPIENTS);

	if (CCRecipients.size())
	{
		for (i = 0; i < CCRecipients.size(); i++)
		{
			if (i > 0)
				cc.append(",");
			cc += CCRecipients[i].Name;
			cc.append("<");
			cc += CCRecipients[i].Mail;
			cc.append(">");
		}
	}

	// Date: <SP> <dd> <SP> <mon> <SP> <yy> <SP> <hh> ":" <mm> ":" <ss> <SP> <zone> <CRLF>
	snprintf(header, BUFFER_SIZE, "Date: %d %s %d %d:%d:%d\r\n", timeinfo->tm_mday,
		month[timeinfo->tm_mon], timeinfo->tm_year + 1900, timeinfo->tm_hour,
		timeinfo->tm_min, timeinfo->tm_sec);

	// From: <SP> <sender>  <SP> "<" <sender-email> ">" <CRLF>
	if (!m_sMailFrom.size()) throw CExceptionSmtp(CExceptionSmtp::UNDEF_MAIL_FROM);

	strcat(header, "From: ");
	if (m_sNameFrom.size()) strcat(header, m_sNameFrom.c_str());

	strcat(header, " <");
	strcat(header, m_sMailFrom.c_str());
	strcat(header, ">\r\n");

	// X-Mailer: <SP> <xmailer-app> <CRLF>
	if (m_sXMailer.size())
	{
		strcat(header, "X-Mailer: ");
		strcat(header, m_sXMailer.c_str());
		strcat(header, "\r\n");
	}

	// Reply-To: <SP> <reverse-path> <CRLF>
	if (m_sReplyTo.size())
	{
		strcat(header, "Reply-To: ");
		strcat(header, m_sReplyTo.c_str());
		strcat(header, "\r\n");
	}

	// Disposition-Notification-To: <SP> <reverse-path or sender-email> <CRLF>
	if (m_bReadReceipt)
	{
		strcat(header, "Disposition-Notification-To: ");
		if (m_sReplyTo.size()) strcat(header, m_sReplyTo.c_str());
		else strcat(header, m_sNameFrom.c_str());
		strcat(header, "\r\n");
	}

	// X-Priority: <SP> <number> <CRLF>
	switch (m_iXPriority)
	{
	case XPRIORITY_HIGH:
		strcat(header, "X-Priority: 2 (High)\r\n");
		break;
	case XPRIORITY_NORMAL:
		strcat(header, "X-Priority: 3 (Normal)\r\n");
		break;
	case XPRIORITY_LOW:
		strcat(header, "X-Priority: 4 (Low)\r\n");
		break;
	default:
		strcat(header, "X-Priority: 3 (Normal)\r\n");
	}

	// To: <SP> <remote-user-mail> <CRLF>
	strcat(header, "To: ");
	strcat(header, to.c_str());
	strcat(header, "\r\n");

	// Cc: <SP> <remote-user-mail> <CRLF>
	if (CCRecipients.size())
	{
		strcat(header, "Cc: ");
		strcat(header, cc.c_str());
		strcat(header, "\r\n");
	}

	if (BCCRecipients.size())
	{
		strcat(header, "Bcc: ");
		strcat(header, bcc.c_str());
		strcat(header, "\r\n");
	}

	// Subject: <SP> <subject-text> <CRLF>
	if (!m_sSubject.size())
		strcat(header, "Subject:  ");
	else
	{
		strcat(header, "Subject: ");
		strcat(header, m_sSubject.c_str());
	}
	strcat(header, "\r\n");

	// MIME-Version: <SP> 1.0 <CRLF>
	strcat(header, "MIME-Version: 1.0\r\n");
	if (!Attachments.size())
	{ // no attachments
		if (m_bHTML) strcat(header, "Content-Type: text/html; charset=\"");
		else strcat(header, "Content-type: text/plain; charset=\"");
		strcat(header, m_sCharSet.c_str());
		strcat(header, "\"\r\n");
		strcat(header, "Content-Transfer-Encoding: 7bit\r\n");
		strcat(SendBuf, "\r\n");
	}
	else
	{ // there is one or more attachments
		strcat(header, "Content-Type: multipart/mixed; boundary=\"");
		strcat(header, BOUNDARY_TEXT);
		strcat(header, "\"\r\n");
		strcat(header, "\r\n");
		// first goes text message
		strcat(SendBuf, "--");
		strcat(SendBuf, BOUNDARY_TEXT);
		strcat(SendBuf, "\r\n");
		if (m_bHTML) strcat(SendBuf, "Content-type: text/html; charset=");
		else strcat(SendBuf, "Content-type: text/plain; charset=");
		strcat(header, m_sCharSet.c_str());
		strcat(header, "\r\n");
		strcat(SendBuf, "Content-Transfer-Encoding: 7bit\r\n");
		strcat(SendBuf, "\r\n");
	}

	// done
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: ReceiveData
// DESCRIPTION: Receives a row terminated '\n'.
//   ARGUMENTS: none
// USES GLOBAL: RecvBuf
// MODIFIES GL: RecvBuf
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-07
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// MODIFICATION: Receives data as much as possible. Another function ReceiveResponse
//               will ensure the received data contains '\n'
// AUTHOR/DATE:  John Tang 2010-08-01
////////////////////////////////////////////////////////////////////////////////
void CPostman::ReceiveData(Command_Entry* pEntry)
{
	if (m_ssl != NULL)
	{
		ReceiveData_SSL(m_ssl, pEntry);
		return;
	}
	int res = 0;
	fd_set fdread;
	timeval time;

	time.tv_sec = pEntry->recv_timeout;
	time.tv_usec = 0;

	assert(RecvBuf);

	if (RecvBuf == NULL)
		throw CExceptionSmtp(CExceptionSmtp::RECVBUF_IS_EMPTY);

	FD_ZERO(&fdread);

	FD_SET(m_smtpSocket, &fdread);

	if ((res = select(m_smtpSocket + 1, &fdread, NULL, NULL, &time)) == SOCKET_ERROR)
	{
		FD_CLR(m_smtpSocket, &fdread);
		throw CExceptionSmtp(CExceptionSmtp::WSA_SELECT);
	}

	if (!res)
	{
		//timeout
		FD_CLR(m_smtpSocket, &fdread);
		throw CExceptionSmtp(CExceptionSmtp::SERVER_NOT_RESPONDING);
	}

	if (FD_ISSET(m_smtpSocket, &fdread))
	{
		res = recv(m_smtpSocket, RecvBuf, BUFFER_SIZE, 0);
		if (res == SOCKET_ERROR)
		{
			FD_CLR(m_smtpSocket, &fdread);
			throw CExceptionSmtp(CExceptionSmtp::WSA_RECV);
		}
	}

	FD_CLR(m_smtpSocket, &fdread);
	RecvBuf[res] = 0;
	if (res == 0)
	{
		throw CExceptionSmtp(CExceptionSmtp::CONNECTION_CLOSED);
	}
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: SendData
// DESCRIPTION: Sends data from SendBuf buffer.
//   ARGUMENTS: none
// USES GLOBAL: SendBuf
// MODIFIES GL: none
//     RETURNS: void
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
void CPostman::SendData(Command_Entry* pEntry)
{
	if (m_ssl != NULL)
	{
		SendData_SSL(m_ssl, pEntry);
		return;
	}
	int idx = 0, res, nLeft = strlen(SendBuf);
	fd_set fdwrite;
	timeval time;

	time.tv_sec = pEntry->send_timeout;
	time.tv_usec = 0;

	assert(SendBuf);

	if (SendBuf == NULL)
		throw CExceptionSmtp(CExceptionSmtp::SENDBUF_IS_EMPTY);

	while (nLeft > 0)
	{
		FD_ZERO(&fdwrite);

		FD_SET(m_smtpSocket, &fdwrite);

		if ((res = select(m_smtpSocket + 1, NULL, &fdwrite, NULL, &time)) == SOCKET_ERROR)
		{
			FD_CLR(m_smtpSocket, &fdwrite);
			throw CExceptionSmtp(CExceptionSmtp::WSA_SELECT);
		}

		if (!res)
		{
			//timeout
			FD_CLR(m_smtpSocket, &fdwrite);
			throw CExceptionSmtp(CExceptionSmtp::SERVER_NOT_RESPONDING);
		}

		if (res && FD_ISSET(m_smtpSocket, &fdwrite))
		{
			res = send(m_smtpSocket, &SendBuf[idx], nLeft, 0);
			if (res == SOCKET_ERROR || res == 0)
			{
				FD_CLR(m_smtpSocket, &fdwrite);
				throw CExceptionSmtp(CExceptionSmtp::WSA_SEND);
			}
			nLeft -= res;
			idx += res;
		}
	}

	OutputDebugStringA(SendBuf);
	FD_CLR(m_smtpSocket, &fdwrite);
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: GetLocalHostName
// DESCRIPTION: Returns local host name. 
//   ARGUMENTS: none
// USES GLOBAL: m_pcLocalHostName
// MODIFIES GL: m_oError, m_pcLocalHostName 
//     RETURNS: socket of the remote service
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
const char* CPostman::GetLocalHostName()
{
	return m_sLocalHostName.c_str();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: GetRecipientCount
// DESCRIPTION: Returns the number of recipents.
//   ARGUMENTS: none
// USES GLOBAL: Recipients
// MODIFIES GL: none 
//     RETURNS: number of recipents
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
unsigned int CPostman::GetRecipientCount() const
{
	return Recipients.size();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: GetBCCRecipientCount
// DESCRIPTION: Returns the number of bcc-recipents. 
//   ARGUMENTS: none
// USES GLOBAL: BCCRecipients
// MODIFIES GL: none 
//     RETURNS: number of bcc-recipents
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
unsigned int CPostman::GetBCCRecipientCount() const
{
	return BCCRecipients.size();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: GetCCRecipientCount
// DESCRIPTION: Returns the number of cc-recipents.
//   ARGUMENTS: none
// USES GLOBAL: CCRecipients
// MODIFIES GL: none 
//     RETURNS: number of cc-recipents
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
unsigned int CPostman::GetCCRecipientCount() const
{
	return CCRecipients.size();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: GetReplyTo
// DESCRIPTION: Returns m_pcReplyTo string.
//   ARGUMENTS: none
// USES GLOBAL: m_sReplyTo
// MODIFIES GL: none 
//     RETURNS: m_sReplyTo string
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
const char* CPostman::GetReplyTo() const
{
	return m_sReplyTo.c_str();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: GetMailFrom
// DESCRIPTION: Returns m_pcMailFrom string.
//   ARGUMENTS: none
// USES GLOBAL: m_sMailFrom
// MODIFIES GL: none 
//     RETURNS: m_sMailFrom string
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
const char* CPostman::GetMailFrom() const
{
	return m_sMailFrom.c_str();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: GetSenderName
// DESCRIPTION: Returns m_pcNameFrom string.
//   ARGUMENTS: none
// USES GLOBAL: m_sNameFrom
// MODIFIES GL: none 
//     RETURNS: m_sNameFrom string
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
const char* CPostman::GetSenderName() const
{
	return m_sNameFrom.c_str();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: GetSubject
// DESCRIPTION: Returns m_pcSubject string.
//   ARGUMENTS: none
// USES GLOBAL: m_sSubject
// MODIFIES GL: none 
//     RETURNS: m_sSubject string
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
const char* CPostman::GetSubject() const
{
	return m_sSubject.c_str();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: GetXMailer
// DESCRIPTION: Returns m_pcXMailer string.
//   ARGUMENTS: none
// USES GLOBAL: m_pcXMailer
// MODIFIES GL: none 
//     RETURNS: m_pcXMailer string
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
const char* CPostman::GetXMailer() const
{
	return m_sXMailer.c_str();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: GetXPriority
// DESCRIPTION: Returns m_iXPriority string.
//   ARGUMENTS: none
// USES GLOBAL: m_iXPriority
// MODIFIES GL: none 
//     RETURNS: CSmptXPriority m_pcXMailer
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
CSmptXPriority CPostman::GetXPriority() const
{
	return m_iXPriority;
}

const char* CPostman::GetMsgLineText(unsigned int Line) const
{
	if (Line >= MsgBody.size())
		throw CExceptionSmtp(CExceptionSmtp::OUT_OF_MSG_RANGE);
	return MsgBody.at(Line).c_str();
}

unsigned int CPostman::GetMsgLines() const
{
	return MsgBody.size();
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: SetCharSet
// DESCRIPTION: Allows the character set to be changed from default of US-ASCII. 
//   ARGUMENTS: const char *sCharSet 
// USES GLOBAL: m_sCharSet
// MODIFIES GL: m_sCharSet
//     RETURNS: none
//      AUTHOR: David Johns
// AUTHOR/DATE: DJ 2012-11-03
////////////////////////////////////////////////////////////////////////////////
void CPostman::SetCharSet(const char *sCharSet)
{
	m_sCharSet = sCharSet;
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: SetLocalHostName
// DESCRIPTION: Allows the local host name to be set externally. 
//   ARGUMENTS: const char *sLocalHostName 
// USES GLOBAL: m_sLocalHostName
// MODIFIES GL: m_sLocalHostName
//     RETURNS: none
//      AUTHOR: jerko
// AUTHOR/DATE: J 2011-12-01
////////////////////////////////////////////////////////////////////////////////
void CPostman::SetLocalHostName(const char *sLocalHostName)
{
	m_sLocalHostName = sLocalHostName;
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: SetXPriority
// DESCRIPTION: Setting priority of the message.
//   ARGUMENTS: CSmptXPriority priority - priority of the message (	XPRIORITY_HIGH,
//              XPRIORITY_NORMAL, XPRIORITY_LOW)
// USES GLOBAL: none
// MODIFIES GL: m_iXPriority 
//     RETURNS: none
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
////////////////////////////////////////////////////////////////////////////////
void CPostman::SetXPriority(CSmptXPriority priority)
{
	m_iXPriority = priority;
}

///@BRIEF	Sets the reply-to email address.
void CPostman::SetReplyTo(const char *ReplyTo)
{
	m_sReplyTo = ReplyTo;
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: SetReadReceipt
// DESCRIPTION: Setting whether to request a read receipt.
//   ARGUMENTS: bool requestReceipt - whether or not to request a read receipt
// USES GLOBAL: m_bReadReceipt
// MODIFIES GL: m_bReadReceipt
//     RETURNS: none
//      AUTHOR: David Johns
// AUTHOR/DATE: DRJ 2012-11-03
////////////////////////////////////////////////////////////////////////////////
void CPostman::SetReadReceipt(bool requestReceipt/*=true*/)
{
	m_bReadReceipt = requestReceipt;
}

///@BRIEF	Sets the sender's email address.
void CPostman::SetSenderMail(const char *EMail)
{
	m_sMailFrom = EMail;
}

///@BRIEF	Sets the sender's name, which will be diplayed in the email.
void CPostman::SetSenderName(const char *Name)
{
	m_sNameFrom = Name;
}

///@BRIEF	Sets the email subject
void CPostman::SetSubject(const char *Subject)
{
	m_sSubject = Subject;
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: SetSubject
// DESCRIPTION: Setting the name of program which is sending the mail.
//   ARGUMENTS: const char *XMailer - programe name
// USES GLOBAL: m_sXMailer
// MODIFIES GL: m_sXMailer
//     RETURNS: none
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JP 2010-07-08
////////////////////////////////////////////////////////////////////////////////
void CPostman::SetXMailer(const char *XMailer)
{
	m_sXMailer = XMailer;
}

///@BRIEF	Sets the username used to log in the target SMTP server.
void CPostman::SetLogin(const char *Login)
{
	m_sLogin = Login;
}

///@BRIEF	Sets the password used to login the target SMTP server.
void CPostman::SetPassword(const char *Password)
{
	m_sPassword = Password;
}

////////////////////////////////////////////////////////////////////////////////
//        NAME: SetSMTPServer
// DESCRIPTION: Setting the SMTP service name and port.
//   ARGUMENTS: const char* SrvName - SMTP service name
//              const unsigned short SrvPort - SMTO service port
// USES GLOBAL: m_sSMTPSrvName
// MODIFIES GL: m_sSMTPSrvName 
//     RETURNS: none
//      AUTHOR: Jakub Piwowarczyk
// AUTHOR/DATE: JP 2010-01-28
//							JO 2010-0708
////////////////////////////////////////////////////////////////////////////////
void CPostman::SetSMTPServer(const char* SrvName, const unsigned short SrvPort, bool authenticate)
{
	m_iSMTPSrvPort = SrvPort;
	m_sSMTPSrvName = SrvName;
	m_bAuthenticate = authenticate;
}


void CPostman::SayHello()
{
	Command_Entry* pEntry = FindCommandEntry(command_EHLO);
	snprintf(SendBuf, BUFFER_SIZE, "EHLO %s\r\n", GetLocalHostName() != NULL ? m_sLocalHostName.c_str() : "domain");
	SendData(pEntry);
	ReceiveResponse(pEntry);
	m_bConnected = true;
}

void CPostman::SayQuit()
{
	// ***** CLOSING CONNECTION *****

	Command_Entry* pEntry = FindCommandEntry(command_QUIT);
	// QUIT <CRLF>
	snprintf(SendBuf, BUFFER_SIZE, "QUIT\r\n");
	m_bConnected = false;
	SendData(pEntry);
	ReceiveResponse(pEntry);
}

void CPostman::StartTls()
{
	if (IsKeywordSupported(RecvBuf, "STARTTLS") == false)
	{
		throw CExceptionSmtp(CExceptionSmtp::STARTTLS_NOT_SUPPORTED);
	}
	Command_Entry* pEntry = FindCommandEntry(command_STARTTLS);
	snprintf(SendBuf, BUFFER_SIZE, "STARTTLS\r\n");
	SendData(pEntry);
	ReceiveResponse(pEntry);

	OpenSSLConnect();
}

void CPostman::ReceiveData_SSL(SSL* ssl, Command_Entry* pEntry)
{
	int res = 0;
	int offset = 0;
	fd_set fdread;
	fd_set fdwrite;
	timeval time;

	int read_blocked_on_write = 0;

	time.tv_sec = pEntry->recv_timeout;
	time.tv_usec = 0;

	assert(RecvBuf);

	if (RecvBuf == NULL)
		throw CExceptionSmtp(CExceptionSmtp::RECVBUF_IS_EMPTY);

	bool bFinish = false;

	while (!bFinish)
	{
		FD_ZERO(&fdread);
		FD_ZERO(&fdwrite);

		FD_SET(m_smtpSocket, &fdread);

		if (read_blocked_on_write)
		{
			FD_SET(m_smtpSocket, &fdwrite);
		}

		if ((res = select(m_smtpSocket + 1, &fdread, &fdwrite, NULL, &time)) == SOCKET_ERROR)
		{
			FD_ZERO(&fdread);
			FD_ZERO(&fdwrite);
			throw CExceptionSmtp(CExceptionSmtp::WSA_SELECT);
		}

		if (!res)
		{
			//timeout
			FD_ZERO(&fdread);
			FD_ZERO(&fdwrite);
			throw CExceptionSmtp(CExceptionSmtp::SERVER_NOT_RESPONDING);
		}

		if (FD_ISSET(m_smtpSocket, &fdread) || (read_blocked_on_write && FD_ISSET(m_smtpSocket, &fdwrite)))
		{
			while (1)
			{
				read_blocked_on_write = 0;

				const int buff_len = 1024;
				char buff[buff_len];

				res = SSL_read(ssl, buff, buff_len);

				int ssl_err = SSL_get_error(ssl, res);
				if (ssl_err == SSL_ERROR_NONE)
				{
					if (offset + res > BUFFER_SIZE - 1)
					{
						FD_ZERO(&fdread);
						FD_ZERO(&fdwrite);
						throw CExceptionSmtp(CExceptionSmtp::LACK_OF_MEMORY);
					}
					memcpy(RecvBuf + offset, buff, res);
					offset += res;
					if (SSL_pending(ssl))
					{
						continue;
					}
					else
					{
						bFinish = true;
						break;
					}
				}
				else if (ssl_err == SSL_ERROR_ZERO_RETURN)
				{
					bFinish = true;
					break;
				}
				else if (ssl_err == SSL_ERROR_WANT_READ)
				{
					break;
				}
				else if (ssl_err == SSL_ERROR_WANT_WRITE)
				{
					/* We get a WANT_WRITE if we're
					trying to rehandshake and we block on
					a write during that rehandshake.

					We need to wait on the socket to be
					writeable but reinitiate the read
					when it is */
					read_blocked_on_write = 1;
					break;
				}
				else
				{
					FD_ZERO(&fdread);
					FD_ZERO(&fdwrite);
					throw CExceptionSmtp(CExceptionSmtp::SSL_PROBLEM);
				}
			}
		}
	}

	FD_ZERO(&fdread);
	FD_ZERO(&fdwrite);
	RecvBuf[offset] = 0;
	if (offset == 0)
	{
		throw CExceptionSmtp(CExceptionSmtp::CONNECTION_CLOSED);
	}
}

void CPostman::ReceiveResponse(Command_Entry* pEntry)
{
	std::string line;
	int reply_code = 0;
	bool bFinish = false;
	while (!bFinish)
	{
		ReceiveData(pEntry);
		line.append(RecvBuf);
		size_t len = line.length();
		size_t begin = 0;
		size_t offset = 0;

		while (1) // loop for all lines
		{
			while (offset + 1 < len)
			{
				if (line[offset] == '\r' && line[offset + 1] == '\n')
					break;
				++offset;
			}
			if (offset + 1 < len) // we found a line
			{
				// see if this is the last line
				// the last line must match the pattern: XYZ<SP>*<CRLF> or XYZ<CRLF> where XYZ is a string of 3 digits 
				offset += 2; // skip <CRLF>
				if (offset - begin >= 5)
				{
					if (isdigit(line[begin]) && isdigit(line[begin + 1]) && isdigit(line[begin + 2]))
					{
						// this is the last line
						if (offset - begin == 5 || line[begin + 3] == ' ')
						{
							reply_code = (line[begin] - '0') * 100 + (line[begin + 1] - '0') * 10 + line[begin + 2] - '0';
							bFinish = true;
							break;
						}
					}
				}
				begin = offset;	// try to find next line
			}
			else // we haven't received the last line, so we need to receive more data 
			{
				break;
			}
		}
	}
	snprintf(RecvBuf, BUFFER_SIZE, line.c_str());
	OutputDebugStringA(RecvBuf);
	if (reply_code != pEntry->valid_reply_code)
	{
		throw CExceptionSmtp(pEntry->error);
	}
}

void CPostman::SendData_SSL(SSL* ssl, Command_Entry* pEntry)
{
	int offset = 0, res, nLeft = strlen(SendBuf);
	fd_set fdwrite;
	fd_set fdread;
	timeval time;

	int write_blocked_on_read = 0;

	time.tv_sec = pEntry->send_timeout;
	time.tv_usec = 0;

	assert(SendBuf);

	if (SendBuf == NULL)
		throw CExceptionSmtp(CExceptionSmtp::SENDBUF_IS_EMPTY);

	while (nLeft > 0)
	{
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdread);

		FD_SET(m_smtpSocket, &fdwrite);

		if (write_blocked_on_read)
		{
			FD_SET(m_smtpSocket, &fdread);
		}

		if ((res = select(m_smtpSocket + 1, &fdread, &fdwrite, NULL, &time)) == SOCKET_ERROR)
		{
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdread);
			throw CExceptionSmtp(CExceptionSmtp::WSA_SELECT);
		}

		if (!res)
		{
			//timeout
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdread);
			throw CExceptionSmtp(CExceptionSmtp::SERVER_NOT_RESPONDING);
		}

		if (FD_ISSET(m_smtpSocket, &fdwrite) || (write_blocked_on_read && FD_ISSET(m_smtpSocket, &fdread)))
		{
			write_blocked_on_read = 0;

			/* Try to write */
			res = SSL_write(ssl, SendBuf + offset, nLeft);

			switch (SSL_get_error(ssl, res))
			{
				/* We wrote something*/
			case SSL_ERROR_NONE:
				nLeft -= res;
				offset += res;
				break;

				/* We would have blocked */
			case SSL_ERROR_WANT_WRITE:
				break;

				/* We get a WANT_READ if we're
				   trying to rehandshake and we block on
				   write during the current connection.

				   We need to wait on the socket to be readable
				   but reinitiate our write when it is */
			case SSL_ERROR_WANT_READ:
				write_blocked_on_read = 1;
				break;

				/* Some other error */
			default:
				FD_ZERO(&fdread);
				FD_ZERO(&fdwrite);
				throw CExceptionSmtp(CExceptionSmtp::SSL_PROBLEM);
			}

		}
	}

	OutputDebugStringA(SendBuf);
	FD_ZERO(&fdwrite);
	FD_ZERO(&fdread);
}

void CPostman::InitOpenSSL()
{
	//SSL_library_init() regesters the available SSL/TLS ciphers and digests.
	SSL_library_init();

	//Registers the error strings for all libcrypto functions.
	SSL_load_error_strings();

	//SSL_CTX_new() creates a new SSL_CTX object as framework to establish TLS/SSL enabled connections.
	m_ctx = SSL_CTX_new(SSLv23_client_method());
	if (m_ctx == NULL)
		throw CExceptionSmtp(CExceptionSmtp::SSL_PROBLEM);
}

void CPostman::OpenSSLConnect()
{
	if (m_ctx == NULL)
		throw CExceptionSmtp(CExceptionSmtp::SSL_PROBLEM);
	m_ssl = SSL_new(m_ctx);
	if (m_ssl == NULL)
		throw CExceptionSmtp(CExceptionSmtp::SSL_PROBLEM);
	SSL_set_fd(m_ssl, (int)m_smtpSocket);
	SSL_set_mode(m_ssl, SSL_MODE_AUTO_RETRY);

	int res = 0;
	fd_set fdwrite;
	fd_set fdread;
	int write_blocked = 0;
	int read_blocked = 0;

	timeval time;
	time.tv_sec = TIME_IN_SEC;
	time.tv_usec = 0;

	while (1)
	{
		FD_ZERO(&fdwrite);
		FD_ZERO(&fdread);

		if (write_blocked)
			FD_SET(m_smtpSocket, &fdwrite);
		if (read_blocked)
			FD_SET(m_smtpSocket, &fdread);

		if (write_blocked || read_blocked)
		{
			write_blocked = 0;
			read_blocked = 0;
			if ((res = select(m_smtpSocket + 1, &fdread, &fdwrite, NULL, &time)) == SOCKET_ERROR)
			{
				FD_ZERO(&fdwrite);
				FD_ZERO(&fdread);
				throw CExceptionSmtp(CExceptionSmtp::WSA_SELECT);
			}
			if (!res)
			{
				//timeout
				FD_ZERO(&fdwrite);
				FD_ZERO(&fdread);
				throw CExceptionSmtp(CExceptionSmtp::SERVER_NOT_RESPONDING);
			}
		}
		res = SSL_connect(m_ssl);
		switch (SSL_get_error(m_ssl, res))
		{
		case SSL_ERROR_NONE:
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdread);
			return;
			break;

		case SSL_ERROR_WANT_WRITE:
			write_blocked = 1;
			break;

		case SSL_ERROR_WANT_READ:
			read_blocked = 1;
			break;

		default:
			FD_ZERO(&fdwrite);
			FD_ZERO(&fdread);
			throw CExceptionSmtp(CExceptionSmtp::SSL_PROBLEM);
		}
	}
}

void CPostman::CleanupOpenSSL()
{
	if (m_ssl != NULL)
	{
		SSL_shutdown(m_ssl);  /* send SSL/TLS close_notify */
		SSL_free(m_ssl);
		m_ssl = NULL;
	}
	if (m_ctx != NULL)
	{
		SSL_CTX_free(m_ctx);
		m_ctx = NULL;
		ERR_remove_state(0);
		ERR_free_strings();
		EVP_cleanup();
		CRYPTO_cleanup_all_ex_data();
	}
}