#pragma once

class Exception
{
public:
	Exception();
	Exception(const TCHAR* message);
	~Exception();

	virtual const TCHAR* Message() const;

protected:
	String message;  // Òì³£ÌáÊ¾×Ö·û´®
};

