#pragma once

class Exception
{
public:
	Exception();
	Exception(const TCHAR* message);
	~Exception();

	virtual const TCHAR* Message() const;

protected:
	StdString message;  // Òì³£ÌáÊ¾×Ö·û´®
};

