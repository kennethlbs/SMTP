#include "CSmtp.h"
#include <iostream>
#define _TEST_GMAIL_TLS_

int main()
{

	bool _isError = false;

	try
	{
		CSmtp mail;

#if defined(_TEST_GMAIL_TLS_)
		mail.SetSMTPServer("smtp.gmail.com", 587, true);
		mail.SetSecurityType(USE_TLS);
#elif defined (_TEST_GMAIL_SSL_)
		mail.SetSMTPServer("smtp.gmail.com", 465, true);
		mail.SetSecurityType(USE_SSL);
#elif defined(_TEST_HOTMAIL_TLS_)
		mail.SetSMTPServer("smtp.live.com", 25);
		mail.SetSecurityType(USE_TLS);
#elif defined(_TEST_AOL_TLS_)
		mail.SetSMTPServer("smtp.aol.com", 587);
		mail.SetSecurityType(USE_TLS);
#elif defined(_TEST_YAHOO_SSL)
		mail.SetSMTPServer("plus.smtp.mail.yahoo.com", 465);
		mail.SetSecurityType(USE_SSL);
#endif

		mail.SetLogin("kenneth.wang@lazybugstudio.com");
		mail.SetPassword("lazybug2014");
		mail.SetSenderName("User");
		mail.SetSenderMail("kenneth@sagaming.com");
		mail.SetReplyTo("kenneth.wang@lazybugstudio.com");
		mail.SetSubject("The message");
		mail.AddRecipient("kenneth.wang@lazybugstudio.com");
		mail.SetXPriority(XPRIORITY_NORMAL);
		mail.SetXMailer("The Bat! (v3.02) Professional");
		mail.AddMsgLine("Hello,");
		mail.AddMsgLine("");
		mail.AddMsgLine("...");
		mail.AddMsgLine("How are you today?");
		mail.AddMsgLine("");
		mail.AddMsgLine("Regards");
		mail.ModMsgLine(5, "regards");
		mail.DelMsgLine(2);
		mail.AddMsgLine("User");

		//mail.AddAttachment("../test1.jpg");
		//mail.AddAttachment("c:\\test2.exe");
		//mail.AddAttachment("c:\\test3.txt");
		mail.Send();
	}
	catch (ECSmtp e)
	{
		std::cout << "Error: " << e.GetErrorText().c_str() << ".\n";
		_isError = true;
	}
	if (!_isError)
		std::cout << "Mail was send successfully.\n";
	return 0;
}
