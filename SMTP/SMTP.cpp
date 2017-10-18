// SMTP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "CSmtp.h"


int main()
{

	bool bError = false;

	try
	{
		CSmtp mail;

		mail.SetSMTPServer("smtp.gmail.com", 25);
		mail.SetLogin("xjwang.hk@gmail.com");
		mail.SetPassword("kxjw28592588");
		mail.SetSenderName("LbsKenneth");
		mail.SetSenderMail("xjwang.hk@gmail.com");
		mail.SetReplyTo("xjwang.hk@gmail.com");
		mail.SetSubject("The message");
		//mail.AddRecipient("friend@domain2.com");
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
		bError = true;
	}
	if (!bError)
		std::cout << "Mail was send successfully.\n";
	return 0;
}

