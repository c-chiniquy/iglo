#include "iglo.h"
#include "igloFont.h"

#include <fstream>
#include <filesystem>

#ifdef __linux__
#include <cstring>
#endif

namespace ig
{

	PrebakedFontData GetDefaultFont()
	{
		// Image file
		const uint8_t fileData[] =
		{
			"\x89PNG\15\12\32\12\0\0\0\15IHDR\0\0\1\0\0\0\0V\10\0\0\0\0\5\xB9\27\xE4\0\0\0\2""bKGD\0\xFF\x87\x8F\xCC\xBF\0\0\0\tpHYs\0\0\16"
			"\xC3\0\0\16\xC3\1\xC7o\xA8""d\0\0\0\7tIME\7\xE4\7\21\16""3*>\xCA\x9E""2\0\0\32LIDATx\xDA\xED\x9C+p2\xCD\xB6\xF7\x7FU#F\xCC\xA9"
			"B F Z \20\10\4\2\x81@DD \20\21\21\xA9]\x88\x88\x88\10""DDD\xBE*\xEA\xAB\x88\x88\10""DDD\4\2\21\xC1\xA9""B \20\10\4\42\2\x81@ "
			"\20#\20#\20\10\4""b\xC4:\xA2\xBB\x87\36\x92\xE7}\xF6\xBBog\x9F:g\xAA\x92Y\xDDs]\xFF^\xBDz\xDD\6\xF8\xBF\15\x9E\xB7w\0(\1\xB8"
			"\xDA\24\xD3#\xD3\x87\x86\xA1\xBC\xA4\26\xD9\xDE\xEBM==\xA3\xBE)kb2\17\xEC\xB9\xF5U\t\xE0\42\xBD\xD1\xF5\xBA\14\20""A!\x82""7"
			"\xF4\37\x8C\0j\xCB<\xD0\x9C\7\xD8sV\xF6\xBA\13\xE7""AP_\27\xB3\xEF]\xB3\xCF\6\xAEVaJ\x8FW\xF9""F\xDAho.NW\x94\xD7\xE6\x86\x85"
			"\xF7\xF5n\xF5\x9A""3\xDD\xD2\x8C\34\0\xE8-<s\xC0\xEF\xF6g\xF6\xDAwy0\x94\x8A\x9A\33{i.j\x9BS\xDB\xF5\xC3\xC4\xF4mZ\x9A\xE8\x9A"
			"\33\xA9\xD5\xC5*\0""D?bdYg\tx\xCB\x87\21xK\xC3\x8B\0""7}\xF3\x96\x9B\xC6*\xE7""0\xDCI_L\x8F\xC9\xAA\xED\xB4z3{0\xE8""7\xEC\xAB"
			"@eS\xDB\24l#\xD8\xDC\33\26\xE2""aU\xD5gks\xF7\x81<\xBB\0\xB8\x9B\xEF<\xEFw\x82\xA4\x87\xB0Q\2J\xB5\x9F\10\xF9\xF1\21""4\xD2"
			"\x7F\6\0\32pS\1r\x8D\34Po\xFD\xAE\xF9G\xAF\xFD\xE3\xF6\xF1\t\xE0\xCD\x9F\xB2\xDD\xE6\xED\x9E\xE5\xD6\xF6\x94\x8E\xE9\10\\H*"
			"\x80\xCE\t\x96\x94W{\x83\xE9\32\xD8|\xFC""D\30\0N88\xFF\xC5\5@\xE0k\4t\xE4\16X\16~\xD7\xCC\x8Ate\30\xC5_\x8F\xFE""7Qw\xE8\xBD"
			"\x96\xB7\xE6""B?t.\3\27\x80\xEDj\x9E\xF2\27\xA5\xAC\16\xD6\xCF\x96tN\xB0\xA4$5s\x83k)R\x91\x9A""C\xDCXB@\xC5\xBF\1 6\x8D\xE7=0"
			":\16 '\xED\xDF""5Q\xF1\xB0Z0\42]>\xBC(u\xBD\xFE\xF2\xCF\xFA\35:\20;\x99""52\xB2<\xF8'\0\xEA\xC7\x86\xD8Y\xB3\xBD\xB3\xAC\xFA"
			"\x87\xEB\x8DUF\xA7\23R2\x9A\xAF}}\3\x7F\xFF\xC8\xF3\32\x97""8\30""B\xF4#\5\xD4\1\x80\xC3w\0\42\xD3hH\5\xF6\xA3\10Z\22\xFE\xAE"
			"\x99\21\xE9""A\17 X=\x9D\xF5;\xB4g\1\xD0/\xD0""9\x94\xE4\xEA\4\xC0\xFB\x98\xED\xA3\21\xFB(H\x8C\xE0_\xAF\xBC""C\xCD\xA8\xC3"
			"\xD3\t)\xB9+']s\x83\xB7""9\x9B\7\\\xE2\xDD\20\16\0\xD1\x89\xDB\x9F\1\xF0\16\35*r)!\xBD\5\xBFkfDzW\xD2/\xBC""8\xEB\xCF\xD0Jk"
			"\x9F\xB5\x9E\1\3\xE6\xA3\24\0ow\xCB\xC7R\xBFM\xBF\xCFWW\x93\xA3W\x86=\xAD\14O'\x9CH\xE1)\xA9\xE8\33T\xE4""2\xC9\xE3\22""5C\xFC"
			"\31\0\x98\x8C\xE8\xAC\xFD\xE3\25\xCBg~\xD7tE\xDA\xB7\xF4""6\xDB\x9F\21\xFB\xA1^\x86\xC7oz\6""4\xE9\34\3\13@+\xC9\xD3\x94\xB2"
			"\26\xFB""6/Z\xF0\xF3\xC9%w\xB1G\xF6\x84\23)0]\xFAZ\x84\26\x9B\xA1~VJ\xAC""5!\xA0v\xA9*\x80\xFCw\0rV\7\xD0\xD9""3zg\xD6\xCBI"
			"\x83\xDF""5S\x91\xDE\x9D\xEE\xA4\16\xD9\xFE\xCC""9\xE5\xC3k\x9E\xC2 .\0\xDC\xED<\xF2\xC9\x8D\5` \42\xA2\xD7\xC5\33\21\21\xA9\3"
			"\xDC\x8A\x88\xC8\5\xD9\23N\xA4@a7)\10\xC0\xBD""4\xF5\xB3R\xE2Q\23\2%-\1\xD5\10t\xE3\14\x80""b*\1""e\xA9\xEC\xAE\xE9\xAE\x9B\7"
			"\x8F\xDF""6\xADH\xAFNts\xF5s\x7F""c\5P\x99\34""E\xFA\xBAg*\42\42#\3@p|QJ\15\xB6\0\x93\x99Rj\xF3\xAE\xA7\x89Rj6 s\x82""C\12\xD0"
			"\x92;\1x\x8B\x8D\xC9\x90\22\xAF\xB1\17""0\x83\xC7\31(\xF8\34\2<\x8F\xBE\1p;K\33qOB\32\xF2""f\xA4\xE8\17\x9BV\xA4_\x81QW+\xBB"
			"\xD7\xB3~\x97\6\xA8\36^\x8Dp\xB7\x94R\xF7\xC7\x9C\6\xE0""F\24p!u($m\xE0y\xEF""CA\32@\xE7\xE0gNpH\xC3\xB9@\xEE\xC5\x8C{J\4\xCFr"
			"m\xED\xD2\xA4\5\xE4>\xA4\16T\x93\xE6""9\0\x85\xDDm\xDA\xE8\xC7k\xF0\x8F\xBB;\xA3\x8F\xFE\xA8iE:\4\xEA\xC7;\x8F\xDAl\x93;\xEBwi"
			"\xAD&\xA5\3p\33\1\4\xC7[\15\xC0""D\xAF{\xDBwx8\6@Q\xAE\xE0q\xAB\x95\xE4u\xE6\4\x87\24\0\x7F%p\xBD""3\xDC\x9E\x88\xBD""1&\xAAsm"
			"/\x8D\17m\xC0\xDB\xF6""8\3\xE0""f?\xF6\xD2""F[\xDE\x81\x99(\x83\xDD\37""6+\x93\xA3\34\x86\xBAq\xB9\20\x89\xDF\xF2\xDF\xFA]Z"
			"\xCF\xCC{\x80\xA9\xB6o\6S#\1U=SZp\xA1\7\xE8\xBA\6\27\x8D\x94tNpH}\xBC\xD0\0\xCF\x9A\xA1'\42\xB0\xCE\xCBP\xBB\42\x81\36\4\xFB**"
			"\xFDw\xFD\xEAg\x8E\xFCK\xB6\33m\xD3\x94n~6\xD4\xFFo\xFB\xEF\xDC\xEA\xD6\35\xB6N\xF7S\xBC\x85m\xFC\4P\xEF\x82H\37\xFA\42\xD0m\0"
			"\xE1\xDC\x83\x89g\\A/\xA5\xD2-\x97\xFB\xE5\xA1\xEF\16\xD5\xC3\42\x8E\x96\xC6\5n~\31\xD5Qv\x82\5\xA7\20\xC1\xC9\xF3wc\5&,\x90>"
			"\xCCKTC\xEB\x92\xFE""f\33\xBD""C\xC5""8\xF8\x93\xC0\xE1y\6\xE0)\x80\xDC\xD2\xF8\xDD\xD6\xE9\16wE\x80\xE2.\4o\xAD@T\xDE\32\30"
			"3\17\12\xDB\0\xBC\x86\7\xA5\6Ph\xE4\x81""F\10""47\xAB\x9A\xE3t\xE3\xB9\xCE\xAD\13G\xD1\6#\x82\xC5\xB2UP\x97\35\xE3.\xD5\xD2"
			"\xF8""B\xFFt\xE1)D`=\x7F""7V`\xC2\2\xAA\xE1\xE1""5B\x80nrl\3\xAC\xC6UU\xBE\6\x9E\xF4""e\xDD\xBB\xD3=\xFB\x8F\xBFp\xCC\1\xD4"
			"\xDA\30V\12Z\x9F\xA7\xC5S\x80\xFE\15\x8E""Cy/%\xE8J\33*R\1\xB6\xAD\xFB""9t\x8C\xDA\xCD\xF7\xD6\xBB\xD5K\xE0x\xFA\26\16\xEF\xEB"
			"\xD6\xB0\xF2""2\xB7:\xAC\xD4\xCE\1\xF9v!C\xBA!\x82""6@\xD0\xFE\x99\xBA\x94\22\25\xE3J\xEB[\x86r\26Z(,O1\x85}\10\xB4\x96\22}"
			"\x82\x88\x88""4N{\xC2\x9D>k\27\xC2\xE0:\13@k\xE8.(E\xB9\x85\xE1\xE1\25:1\xE0\13*\xA6`\xC2+\xF9hq\xA1Z\x9B\x85\x7F\xEE\xD4\xBB"
			"\xFC""E\xA9$\x87\xFB.\xD0\x8B\xFC\14\t^g\xB1\x8BV\37\xF6\xB1J~\xA6\xFC\xC3\35\x8F\33\x80\xE0""3^\xDF\xCF\xC0O\xCC`\xA5\x8CMR\1"
			"\xBB\xFB\4\xD8?\xAA""b\23""D)\xA5\xFC\xD3>\xC3\xF0""Fe\xDB""a\x9CYQ7}X}\x8C""a4\0\xD8UZ\13>\xCD\xBA\xDC\x9B\xFB@n\xD3\xB1O]F"
			"\xA3sVqd\xF0q\x9F#<^\x9F\x91\xC1|\xD9R\xAA\xF1\xF6;\0\xF8\xFC""4K\xEDpX\xAA\xAD\5x\x91y'\xC4""a\xEC*\x9DW\x8B\6\20J\x8EoA\22"
			"\35\x9F\xD1\xFB""c)k?\xA6\xF4\xA9\xF3m\x8D\x7F\xACna\xDF\6\xB8=\xEE\x9B\27S\xEB\xE4\xEB\xC1\xBD\xB1\xAE~A\xEEt<\xC0\xE1\xCFK"
			"\34u\xB8\xE9\xD2\x9B\x9F\x93\xDD\xB9\xE7N\xBA_\3p\23\xFB\xC7\32\xC0""1\17u\1h\x8D\x8F\x87\x96\xC3\x98\xBF\xCD\xD9\xA0\xA5v\tW7"
			"\xFE\17\0\xE4\x8F\xC6\x8C}=\xE6""9\x9E\3p\xCC\0p)\xF9\xCAW \xB9\x8A\x84i\10\xB1\xA4U\xA1o\xA6\xA0\xB2Q\xDE\xFB(0F\xA9\xC3\xDF"
			"\xFE\24p\xA5\xB9/\35*\xE7""d\x94\x8A\xAD""a6\xFA\5\x95K\xEE\xB7\0\xB9\3\x90\x8F\xCD\xBC\32\xAD])\xEBu2\xFB\xF2R\xB6""5=Ef\x9C"
			"\xF6\x7F""F\2\x82""c\xEB\xFA\x9D\xA8\xD6Y\xA7\x82\xFC""bT!\xAE_\10""0\xED\xF3\xF5q\xCE\xDF\xF0\xD3\xD1R\xD3\xF8\xE3\x9CtE\xE4"
			"\x8F\1`\xBA\xD5n\xC8\xD1\x83\x8A""E\xBD\x94""8\x8C\xD9\x91O%\x81\xEA\xD7JO\21;UB\xCE\30\x8E\xC3l[m\xB2n\xC5\xF4\xFD\xA5\xC3"
			"\xF8~\xD2\xB3+\xC8*\xD0\xAA\20v\x9A\xC5+\23\21\14\x92""6/\xDBsVK\x87\xCF\22\x94.\xB5\15\x9B\xCAQJz\2\xF4""E\xE4\xF7\0\xDCk7"
			"\x9A\xAF\27?\x9CGP\xAB""Ce2s\31\xD3s\xDF\xD1\5""6z\x9A\xD5\1\xEE*0le\1\xB8\36""d\1xXN/y\xFD""8\\\x9AkG-\xA3\12\xE1""c\4\xE0-"
			"\x9E\xACS$\42R\xE3\x8C\xD5\xD2\xE4(\xB2\xEF\x9E\xC7\xCBSr_\x85\xBC\xAA\x8B\15""5\xA8\xF8g\12\36\xB4""3Le#\xBB\xD7\30\xAE\xF7"
			"\x92\34\6""aF\xD1j\xEDoV\x83R\3\xFFy\xF9\3\0\xAE\35p\xD3\xCF\2\xF0\xD9\xCA\2PJv!\xED\xDD\xD1\xAC\xF6\x97\23\xA3\12\1\xB5\37\27"
			"\xA9|E6+\xB0Q\xAAj2\3\31\13\xC4\xDF\xF4\xE0\x97\0\14\x87)\x8BI\xC9\6\32\x92\22P_e\xFA.\17\xB7\xA7{^-\1\x9A\xBB""7\xB2+M\xB0\17"
			"O\xF6\xC0\xC5.\x91U\xEDG\tp,\xC1\x99""2G\xF2J@\xAD\xBD\xB3\xD0""B\xB4\x87\xAA\xD8\xC4N\xDE\xB1""6)\x8F\x8Fr\xE8\x9B\xA1\xF6"
			"\xF6=`\xBE\xFA\16\x80\xAA\xC7\x7F\0@\xF9""0P\xF8\x97\21""0\x9C\27)\xAE\xBB\x86*\xA5Tq\xDD\5?\xB6\xE1\xF4\xEB\xB2\xBA\x8A\xEF"
			"\x80\\\xE9""2>\3\x80\xFE\xA3k\21\xEA\xA7|3\x84""2\xBE@#\xEB\13\xD4\xCF\xEF\xF8\xD7n\15i\0\xB7\xC6\xB1v\1x\xB6\xB1\xDF\37\1\xA0"
			":=\x8A\xC4\3 \xFC<\xCA\xE1\xD5\xFB\5\x95""B\xDF\xDD\xEE\x97w\0\xE5\xADNQ\x9D\30\xD3>\xC0\xC9'\xF0\xFE\x85\16q\xB9\xED\1\xB9"
			"\xB6\xE2\17\x8C\xF0\xFF\xF1[\x8A\xF5x\23\xD4\x81\xFE\xFF\xB0W\x97\xD4\26\xEA\3\xE1p\xBB\35\x84\20\xAE\xB7\xBB\xD5k\0\xA1\2\12:"
			"\6Tl\xFC""dDZ\xA3""3\xE8\x97\17\x93?1W\xBC\t\xC6S,}\xC5\xCF.p\27\xF5j\23 \x9C\xEE\xDE""A\x9D\x92\xB8y\xED)\xE5\36\xBF\xA2\xEDW"
			"\xC7\xCFz\33\xE6\xBC|\xC1u\x97$\xE5""1\xDC""D\xBB\xF5{\10\xE8\xD5'\xD7\5\xA5\xEA\xA2\x94rg\xE0l\xA0\xCA\xE3\t(Q\x85\xE6z\0\35"
			"\xA9\xA2\x92\26\x80\xB7\x91\37\1""8\xCB\xE1&.\x93L\xF8\26""B\10\xDE""7\xD1""B\xFB{\xDAn\x9C\xBF\xA9\xBC{\x9Fq\xF3\xED\36\xA0"
			"\xFF\xA1\32\xB0\xEE\3m\xB9\6\xBE&\0\xF5\xA8W+\xA8\xC6\xD0""d\xFDm\12qa\xD2\xE2""C\xD7X\x96\x99R\xAA\xB8\26P\xA2\xD4\xC5\xD7(}a"
			"%\xDF\24\xB3\x80/>\x94\16\xE6@e\xA7\xD7\xABW\xFD\x84\xFB\xF8\34\0_\5\xA7\xF6\xFC""f\xB5_\xD4mJ\xC6\xB8\x80y\xF0\xF2\xED\2\xB4"
			"\xF1\xF0\x82""6\xC0\xC7\xB4\xA4\xC4US\x92\xCF\2\xB7\xCB/\xAA\0\xFB<\xC0[\4\xF4\x93w\10\x92{@m+\xDE\xE3*Z\xE4\xFA\xAF\x99\24""b"
			"\xC7\xC4Y[\31\0\xDE\xAF\xE0\xE1\xDD\xE6LK\xFB\xDF\0\x90\23\xB4I\xA9,\xC1\xCD.\xD8\xD5\1r\xBB\xCE\31\0\xDEt\xEE\0\x90\x8CK\xE5"
			"\xE1\12\xA2\x87u\xFC\31\x9C\xBB\x80\xCE\xE3\16\5\20=u\xB4\xD0\x8A\x93\xCB\2\x8A\33\xFF\xE0\x9D\x92RMQ\20\xCD\x96p)E\xE0\xE3"
			"\x9E\x97""e\xF5\xEEHu\x9DI!\xEA\4\xCC\xD3\xCE\xCB\0\x90[\25*\x8B\xA2\5\xA0\26\xFD\2\x80\xE0#Z\xBF\t\x88@y\xFE\xA4\17\xD4\xD6/Z"
			"\xA6\xB4l=\xC7\xC5""3\0\6\xCB\xC0q'$\x84P \x99\x96k\xAB\x8Fso\xF7\xF4\xB8\xBC\xB1p%\5^_\37\31\t\xEA\x9AI\xAB\xCC>8\xB6Qr%9\x93"
			"J\xDB\xE7\xD8U\xE9\xAC(\xC4\xD9l\xE2x\4,\xDF""2\36\x84\xD0\32/\xAAJ\3\xE0_n:\xBF\0`0/W\xBE\4\xEA\xC2L\xFA\x80/\x89\xC4\xCF\xDE"
			"\xE9""5(\34;*\13\xC0\xF3:\xC4q'\xC4\x82\x9E\x87\xFA\xE6\xDC\xDBM\37\xA7""D$r\1\xF0\xEA\xA2""B \x99WJ\x83\25\xE4\xDE^\37?\x95"
			"\xEE\xAF\13""0\xEB\xD3\x8E\xFC""c\x93\xAF\x9E\16\xC0\4\2\xEF#\32_\xD9l\xE2\xCD\xD1Gi\xBB\xDB\1\x80\xC5\xD0\xCCn\21\31{\xB8\x8A"
			"\xF1\4\xC0\xB1\xA4\xDDi%\x84\x97\xEB. 9\xCF\x98\15uQ\12\xE8\xC7\xFE\31\0\xF2\xF2M\31Z7C\35\xCF\xBD\xDD\xC8>\xCE\xAB\x8B* \x8E"
			"\xBF""f+\32\12P\20`\xD6\xE8\xB7M\xBF\22\xE0""1\xA2\xDFg\xF6\22$\27\0\22p\xAC\x96\xF7""C\x7Fz\x9F\xCD&\xFA\x87\26\x9D\15g\0\xD4"
			"\xBF\xD6""E\35""E(\24\37\xA5m\x97-\x8D\xAD}vhES\tP\x8F\xC8\12\xA8\10\x90\x88\x88\x88r\30\xBE\xD6Y\xA9o\0""8\22pr\1S\0\x8C\30"
			"8\xFE\x9AS\xD2\x91*\xC7\xB6\3@UJ\xDB""6\xDD\xE5\x85\16\xAD,nx\x96\xE8\xE9x|\xCE\xA6\20\xA1?`\xDEM\xDDI]1\xE2/K\x8DY\32""F\x99"
			"\xF6~1\5$\17\xE5TU\36\34\xB6R\tPJ\xD5""Ey.\xC3\xCF\x87\xEAO\0|\xF8\xF9\xAF\xF7s\27\xF7\7\0\xCE,`{\xAC\26\27\17\xA5\xC0\1\x80"
			"\xDD\xAB(\32\xD2\33\xEB""0iT8\xC5\x9D\x9D\24\42\\\34\x8A&B\x9A\xBAK\xF2|\17\xAF\xCF\26\x80\xC7\xE9/\0\xF8\xFA\xCC\xE5'\2J\2"
			"\x82\xE1""4\xB3\xBA+\xF9\x85\33\x81""7\x89\xC3l\26\xCAS\xF0""5<&\xC3\xE0\xFC\xEA}Z6\xF2;\0Z\xA3\xCA\22w\12""0\x88#7w\xF9\xB8"
			"\xEF\24\13\xB5\xDE\x85\xAE LS\x88\xC0vjBq\xA9\xBB$S\x9BWD\x80\xDA\xDE\xCB\2`\35\x83\xD2Z\16\x8F\2J:\42\xD3R&\x85v\2 \xFF""7X"
			"\xB9\xA9\x96\xA9\0\x97\xCB\xBF\2\x80\xD7\x87\xFB\xB7,\0m\xE9\xBB\xC9J\32\xE3\xD8\32\x82N\12\21x\21\x83Q\xEA.9\12OW\xCC\xF9\xA0"
			"\xBD\xD8p\xF6\xAF""1\xB7\xD3uf]\xA1\26\xDD\xBB\0XC\xC8\x8C\x82\x95\xA0\x9B""b\xB3j\xDD""6O\xF1o\xB8=\xAF\xE3y\xD5\4\xAD""D\42"
			"\x80\xE2t\xB7\xB9\xD6\xE5r\xDB\xE8\3\xF0^\x92,\0\xC1\xE0(\xFB\27\xCF""0\xF9\xEF\xC5\x98\xD7\x9E""E\xBB\xCD\0>\xF6""9\xF0""7"
			"\xEF \xBD\xF4\xE5\x83\x8F\xED\xFA""a\6\xAB""90\37\3\xD3z\xB1\xB7\0\xF2J\xA9\xFA\xB1\xAB""c\x9F\xC5j\25X\x8F\xEB\xAAt\15\xA5"
			"\xE5\xF1\37\xEC\xFB\26\xDE\xD7\xF1""fp\xF1\14\xE0\x8Dzn\xFF\xA9""0\xD7\33\x9A\32\x8Fp\23""E\xD1\xF8\xCE\3\xEA\xD2\6\xDA\xC7\42"
			"@{\xBD\xB5%\xB0\xFE\xE3r\33-;\x80?[]+U\xE9""A~\xFF\2Oq\16""D.\xD3\xBA\36""c#\xDDK\1%M\xED\xC4\24\xF6""6x\xB5\xF6\1t\xC5\26""ab"
			"\xD4\xF3lP\xFD\xEB\1hI\xBA\xF2\xC9\xD6\3\xF0""bk\27\17M\xB4\xDET\42\x8A\xAE\xC7\15""c[\xC3\xEAV+\xC2\xCB\xEB\xB0""c\xE4N\xA9"
			"\xF2}4\17\x80\xC1\xC6\xC3\xDFh\xE4\x8EM[\x91\30,\x97\xAD\x82""6\xC6\xBBi%0w\xC7""B\xF1x\15""D\xD3""8o\0\xB0""6R\xEEx\xCF\xD3"
			"\xD6\x9CZ7\21\xC4\xAB\xA4\6\xC6\x85\xC0IL\25\xF5\xB5""9rf\5LM\x8B\xD2\xC7z\xBB~\xAB\xBB\1\x9B\xE5""f\xAF\x94R\xDA\xAC\xBE\xD4"
			"\x90\30\0:6\x8B\xF8\xF1\t\4\xF3\xFD\7""4\24\x84y]!\xECV(\xBA\xFDz\xE2\5\x8B' \xDC\xB7\xE9\xE8\x8C[ ^\xC9\0p\xCAO\xBAY\10o\xD1"
			"\x9FLte\xD4""a\xA4o\x93\xDAH\xF4\xA7,Ma\37\x93WSx\xF6""fl\0\xB3\xEC\xBD\xC8\xD7""c\x88\xB3\xE4\xBBs]\x80\xCA\xE1\xBD\xA2\xEA"
			"\xC3\xC3)%G\xEBpk/G\xE6\x9F\0\xE3\xAF\xE8\x8C\xD1}\31\x82y\xAF\xB9\xD2no\xFC""e\xFC^\xA7""B\xD1\xED""7\x9A\xA7\xB5\4\xE8l\xF2{"
			"#1Qgn\xBC\x94S~\xD2se\xB5\x96\34\x95~\xD7\xB6\xDC\x99<\x94\xB1\x91\xA8%\x8D\xC4\xA4n\xEFvZ\xE4\xFBQ\xA0""C\x7F\xC6H\x84\xD6"
			"\xE8pl\x9D\1\20""9\0L\xB4\xF9\xFC\xE9<t\xF9V\25\x9B*\x92\x87""c\16\12I'\xCA""2\24\10x\xF3\36*\x82\xEE\x9Er\x92\4""9ig+\24\xDD~"
			"k\xE1\xEEu\xDEjaS\xF4\37\xF2\25\x9CG1\2\xF1\xB4\7\6PN\xF6\xA1""9\xDC?T\x94""86\22\xAC""6c3\1\x92+\23\xF3""4Y/\xD5\x88\x95""2"
			"\xB3#\xF7\xB1\xFD\1\0k\x89$\xA1\xB1\17\35\15P\xCE\xA7\xBE\x89\x84\xC7;\xE8\xC6\xA5""3\x86<\xC1\33\16t\xC6\xA9*\x95\xCEj\xDFjI>"
			"[\xA1\xF8""C\xBF\xD2&rZ\x8C\xDE>\xCCW~>\xBA\xCC&\x9F\16u\xC8i\xDF\xC0\xFB\xEA""E\xB6\xEE""2\xD8\xAC\13""bl\xA4\xF7\x83\xCE\xC2"
			"h~K;=\1\x82(5\x8CJ[g\xE5?\xA4\0\xE4v\xB9,\0VY\24N\x99\xAB\xE5\24\16]\xAD\x8E\x94""0\xF8\x82\xE8YI\x96!\x8E\xC5\xE1\xD0""3\xC9"
			"\x80mw\xD8\x9F\xBC\xEB(\x8D[\xA1\xE8\xF6\xEB\xE7\xB4'YSn{\x99\xDB\xBC\xDDJ\10\xECK\xE9\13\x9B,\x84\0\x9Dmp\x95""FT*\xC7g\xC9"
			"\xA4\34\6\33\17 \xB7""6\xA9\xBDW3\21 \30\xEBu\xA9V\x87\xD2h|zh\xC4\x99\4\x98\xE4i1\xF5\x84\xF4\22\xF0\x9A\32\15U\xA9\\&\xA1"
			"\x92""3\x86\xE6\xBB\xA1\x87\xA9""D\xFC\30oo\36W\xFA\x83\4\xB7""B\xD1\xED\27\x80""0\xBA\xCA\0\xE0\t\x94\17\xBB\21\xC0`p\x8A]\37"
			">\x8B\xD0\20P\x87\33\x98ns\xE6\xFA\xCEQ\x80vU]\xC6\xF7P\34\xEA""D4Ov\xC6'\xA9\x95""91\3z\xBD\27""9\14\xC3,\0\xB9]\xEE\4\x80M"
			"\x9F\xA7S`9SJM\x86\xA7\xA9\xBC\xE8\15\x86\xFA\\\x97\xA1\x9E\x9C*\21\x9B\xFB$WI\xB4\xFAs+\24\xDD~\x81\xB0\xB3{;3\xE6\xB7\17^"
			"\xFEKnt~rP<%s\16\42\xFB\xA9\xF1,J\x89\xADMd$\xC0\xE3\xE6\xB0\xBA\7\x9A\xD3\xEA\x9F""7_\42\xAB\10,\0\xEF""C]-#\xA9\6h\x80\xA9K"
			"\xD7\0\xDCl\xF6\xA6:\xD4""e\xE8\xEDv!\xB6\22\xD1?\xCE`kT\xABS\xA1\xE8\xF6'\x92\xEC\xC7\xCDso\xA6\31K2\xF9""8\xD4\1\xCA\x93\xA3"
			"H|\xFFO62\xBF\1\xA0v\xFD\x90\xE2""8\x96T\0\0:6\12\26\x81\37/\xCD\xB9\31""F\xFF\x9D\xB6\xD2\xCA|,V\xDB\xD4\x81\xB2\x88\x98""e"
			"\xB6\xB8\xD4\xEB""d0\xBFt\xDD!\7\0\x8A\xE3\xA3\34?k\xAE!\xF4\xDF\xEF\13\xBCn\xA2%\xF0\xBC\xDE~\xB9i@\xEB\xEA\x98\x9A\xBDy\32"
			"\xED\xBD\xDA\26\0\xBC""es\x99\3\xB9U\xB7""b\xEC(S)\xF4\xF9\xEEH\x80""6\7\xFF\xC6\xEC\xE2\xB7m\xBC\12\xEAz\24\xD2""2$^u\xA5""a8"
			"\xD7\x8B\xBC\xF7\36\xEF^\xBC\x93\xBB\xB9\x89\xE2\xCD\xA0h]c\x80\xF1\xCC\xAF\3\xBCo\x9F\0^\xE7\x95\xC2\5\xF0Z-~\x9C\x8A\xCA\10"
			"E)U8\xD5\xEC\xB9\xD1\xDEo9\xA6\xE0\xEF\xE0\xE9~\xB5]7\x80`\x92~iV\xECY\xA3\xF3}\xE0\1\xF8\xD3\x91\xDE\xA7\xDF\xED\x95\xB7W\xE6"
			"\xE4\x87u\1 \xF8\xBA}\x9Ez@w]i\xC4\xB7n!X\xA1\xFA\xF9\5J)5\x9B\1""A\xFF""2\x99\0\xDE""D> \xFD\20\6\xA0p<\xC5\xE3\xD5\xDEI\xF0"
			"\xE0\xC4\xFA\xEE\xD6\xF1\xB2\16\x84\x9F\xDB\xA9\26\xF9+\xBD\xEA\x98\xB6=\xFE\xE3""f\x9DK\1.\x92\33\xE0""6\xBE\10/J\xDA\x9A""03"
			"\xE7\xEA\xD0o\xE8%\xF6n\xAD\xF7\xBD\xC7w\x9B\t\xD7\6t\3\xF0\33?\22\x9BK\xB8\x9D""fcuE\xCDL\xC9\24\x94\xD8\xB4\xB2\xA7\xCBXN"
			"\xAFw\xB1\xC2V[\xD9o.R\x86\xD2h\xEFUY\xF5V\xC0\xE7\xA0\xF8>\xD2\x99\x8D\16N\xDB\36\xD7\xA9\xC6}?\0\xA8/\xE2""E\xCDq.\5J\x87'"
			"\x80""E\xFB\x9B\x92""9^\xFD\12\xBF\xFC\xC7""6z\15\x9C\xF8\xD1O\x84\xAF\xBF\16q\0(\x8D\xF5\xA4\xEC/\35\x86\xE8""D\xF1\xA2\x9E"
			"f\xA6\0\32Q\33\xBC\xC5\xBB""1tw\xEB^\0\xB4+\xAA\xB7\xCA""F{\xF3\2\34\xF2:7\xC1\xBB\xAE\0M\xDBi\xB2\x87\xE9\xA8\\]>\1\xC4\xF7"
			"\xAA\xBBt\x9CK!\xDC\xE8\xF9\x94\xE4\xCE\x99\xEBw\xCF{Rb8)W\27\xAF\x7F\10\x80:\2\xBES'%;\x91\x89\7\xD8""2\xC4+\xCD\20\xCDR\xF1"
			"c\5L\x87""9\x8A\0""3\x99\xE5\xA1""c\0\xF1\x95\xBAZ\17\x9C\xB2\42\33\xED\xAD\xCD\xB6[\x81\x9C\x88\xC8\36@\xC7""d\xD3\xB6""9\16x"
			"\x92\x83\xFA\22|\xFB""5S\xEA\\\x8A?\xD7/\xD5\26\21]\xBC\x98""2\27\xF7\xD6\xF1\xB4\xE8\xF6X\xC2K\12""f\24\42\xCB\xF7""7\xA2\xB0"
			"7\16\xFA\xA9\20\xACx\23\xBD\3\xBCl\xAC\xE7oGH\x87\xC1\27&\xB0\25\xD6\xC6S\20G\xFE*\xF1\x89!\xB1\xD1\xDE}G{\16\x89\xEFi%iT\xA0m"
			"\xDB\xE3\x90;zP\xDA\1\34\32\xDC.\34\xE7Rt^\15J\21y\xA5\x8B\xE3\14s\xBE\x8CJ\xD5\xF9\xC4\xE9I\x89\x82\xFC\x92\xEF\x94\xF0\xC4"
			"\x83\xE2""6\xAB\3j1\20\xEC\xB5\xDD""cG\xE8rnF\xCA_\xBFY\xED\xBB\xCB\xAEV\xC5\xFD\x89!\xDB\xEF\x8B\xF2^\4X<{\xBES\xF2h\xDB\xE9q"
			"`\xFB\26\xE4\x86""1\xC0}\xB2\x90""7\xC7\xB9\x94""d\xAAG\xA3k\xE3\x8A)s9\t4liOJx\x92\x87j\14\xC1\36\xA8\xCAO\4q]\x97\xAA""f\22"
			"\26\7\xA0""cJ\xCA\xEC\10\xED\xAFm~\xA8\xBE\5\12\25\xBC\xEE\xFC\xB4\xAE""7\x8A\24\xC6\xA3\23""C)0\xEFr|\x89\x80\xDA&I\x9E\x9C\5"
			"\xDE\xB6\xEDq\xE0""2\x96\xC3p\t\xDC%\xA3y\xD2\xB4\xCE\xA5\x80\xD4\16\xDA\x97\xCC%\27Z\x8ER\xE6""8\xE4\xA1\30;=\xA7""C\xD3~\x90"
			"\x9F|\x80w\xECx\xE1\\2\xC4\xBD&x\xFF\xCA\xA9M'\xAB\4'#\xF0\xB6\xDD\xCC\10\xFA\xFB\x9C\xFF.\xBA""b\xA9""c\12\xC8\xBEJ\xDA?+\3"
			"\x8F\xC7$\31\x87'\x86\xFE""6\x9B\xD4\xA7\xF7\2\xC4Mx\\e\12\25/\23\xFD>\xE3YH1p\x98""c8\10\xF2\xA3\xA1\xDB\x93\22j*\xC9""0\17"
			"\xDC\37""dw+?\22\xF9\x99\xC8\xA7""c\10%\42\x87\xCF\20\x9A\xF6\x93~;B\3\xD9u\xFF\xF9\x96\xF6\xD5""A\31\xD3\42<f\0\xE0V\xDA\0"
			"\xE1""8\x91]\xD9""e\xAE\xB8\24\x99\x86nOJ\xFC\xFBl\x95\x91rC\xE5g\xC6\xD1\xA9\10\xFD\xEDp\5px\xF2\x82\xDE\x97""5\xB7\xCC\42"
			"\xFD\x9C\\\xFC;\xB1t\xB7\x89\x97""7\xA3(\x9A\x84\xFA\x93\x80h\6\xB3\xA9\xB3\10""7F\x95\23\xFB\xCB\26\xF0 !\5i\xFE""d\34\xE9m"
			"\24\17\xB4\x9AlmE\26\xE5\x7F\x98\xF7""b\xEB\xABl\xE1\x91\xDD\27\42""0\x9F""6\xCD\xBBqt}\xB3\xDC//\x9C\xAC\x93-\xE3Y\x8C\xB7\5m"
			"\x96n\xDFr\37\xD1\xF6""3\4\x9A*\xF7\42\xE3ry4\x80^DQ\x8E~p\xEC""8\x8B\xB0\xDF\x89""5\4\x86}\10\x93[:6T\x9E""5\x8E\xFET\xD2""cU"
			"4Bs\xAA\34""3\1""F\xDC\14\xE6\xE9hZ_\xE5""e\xF7&\xE3\xD9\xF0 \xE9\27\xBB\x87y\xA5\xF4\xB1q\x8C$[\x84!]\xE5\1\xC3y\xA5\xBA\xDE"
			"\xCD+\xC5\xD1\xD0\xF2\x90\xD7(6\xA4t\xB7:\\^\x8Ar\x8D\17\13\x81\xA4\x89\x99\x91\15\x95\x7F""3\x8E~S\xA0g\xFF\xE9O\xC6n\xFA\xFA"
			"\x87\15\24\x81R>\xAA\xA6\xE5n\xFB""9\xCF\xB9\1\x8D\x86\7""A\xC3""5\14\xCF\xDC\17""e\x96T\23\xCD\xCEI\xC1\x98^i\xEA\xDE\24""a"
			"\x88^\17\x93\x82\xFE\xEEH\33O\27\x8Bx+\42\42;\xF0v\17\x83\xC1\xF4\xEDy\xE9\xAE\xCF\0\xFEP\xA0\xB5\x9C\31W\xBE\x99Tt\xA8\xFC"
			"\x9Bqd\xC7\xD6$\35\xCA\xEB\7\x80\xDB\xF5~\4\x9E\x9CJ\xCBj\xD6\xB5\21X\x8C\xB8\22\xA9""3\32\32\xB9\xDBg~~\xE3""d\xF5\xA6\26\xA2"
			"\x9Dq\xCDJ\xF1me\xABO\x9C\10\xA4\xBD`\xEBV\xAFH&C^\20;h\xBBv\xA1q\xF4\tU\1\xE8\x8F\xA3vw9\xEB""f\xD6gh\xCC\xB4\xF0\xB7\xE6\32"
			"\2/^\x8E\xFF\xD0""8\xBAYj\x87k}\xB1\xA8\x82wl\xAAJF\24\32)k\2\xDD=O\7\xB9g\xDF\6/)x\xC3\xC5\xFAG\0N2y\xEE~8\x99v[k\x9F\xB1\22"
			"\xB3\0x\x92\x83r\22h\x9E})\xF9\xEF\xC9G\36\xBF\14\xB4v\x92\xAF&\xFB\12\xEE\42\xDC\x98\xCDR\xB5\xDD\x98)\x80W\23*?3\x8E~\21\xFB"
			"\xF0\xCD\xE7""3\xA7\xE3\xE2\2P\x95\xCAg\x7F\xF2Q\x91<\xE4\xC4\33\16\x8B\xFB\xB4\xA6)r\xAD\xFD\x93\xCDx\xE6~\xA8\xC8\5 -HI\xAD"
			"\xC4,\0\xCC\xFA""An\x94\14\14\xCF\xEF\xC9\xFE\xBD\xB1\x92\xE3\xF1\t\10\x8Es\xD8\xEAh\xBF]\x84\xEB\xA3\xCA\x9FUo\xE7!\x82\xA7"
			"d\xDB\xFC""5\0l\xEF\xD7\x8D\xD6\xD7\xE3\\\x97\35\16\xBDj\x8C\xADg-d\xAC}+\x93\x89\xEFg\xDC\x8F_\0\x90Z\x89g\0\24\xA7\42\xE3"
			"\xF2\xD4\xF0\xFC\xCF\xD8\xBE\x85\10\xC2\x97\xF8\17\0\xF8\xF8\xDC\xE0\xC5\xA3'\x80\xAF\xC8\xC8]*\xD8'k?\x95\xC9\xC5\xB3\x97+:3"
			"\xD0\x93\x82\xAD\x93\xB4U\10\x9Er\xAC""D\xDB\xF1\xF7\x96'\14\37^G\xA0\xA4!\12h|6>\x81\xDEg\xEF\23\xC6\xF7o\xAF\xA7\xB4\xE8\xD9"
			"\34\xCD\5\\o-\xDF\xFF)\xDF\0h\xEE\x87\xB0\xD8\x97\x81\xD2|\x95\32\x7F\6\x80\x93\xB5\x9F\32\x86\xB5M\x92t\\\xF7\xE3""3Y\xFC\35"
			"\xD1G\xAD\xA3#=\xE3\4@\xFF\xC0\xD1\xA9\xCE\xA6r\14\x81\xD6\xA2\xB6\xCD\x83\22\xFD""f\xB3\xAB\xD9\25\4\x87\xFA\xA1\2j[\xDE""6"
			"\xEC;\x9F\xCF\xD1Z\x92\xE8H\x9E\0\xFF!\xB9s\0\xFC\xE3\x83\xA9\15\xC6s~\xE8\xCB""7\xAA""2\xB5\xF6\xFF\x91\xE1\xE0\5Lg\xDA\x9E"
			"5:\xBA\xA0\x8A\xEB\36\10\\\xE6\xD6\xDD\xB0\xA1\xA3\x89:y\xF7>y\6V\xCD\xF1=\xE0\x89'@#\xBE\x88=\xB8\33\xDD\x8D\0^\xDF\xEEWiv"
			"\xF2<D\20\x9A|\xBA\0\xFF\xEF\xFF\x7F\x93\x80\x7F\xF8""6Y\4\15\33\xD6~\xDE\xDE\x81\xB7\xB9\3\xEB\x80\xF3:\xAF\24jPJ.ov\5GG?\xAD"
			"tus\xB4_~\35\xD7\xAE""a\xB6\xCAG>\xDC\xCC.6:\xEB\31\1\xB3\xD7\xD9+\xF0U\xF9\xAA\0""aT]\xB7\xD2\x9C\xF8y\x88 \xACo\x7F\\\5\xFE"
			"\xE4""f\xA6\34\xD0\xEB\xC1\xF8\36\xB8\xDD\xCD\33\x84\21\xF8\xEB\xBB\xCF\xBE\x99`~\xBBv\x98\xD8\xB0\xB6""4#P\x97\21\xD8hw\32\16"
			"~^j\xC1""4:\xBAr\xAC\xE8W\xCB_'\x87\x8B\x9C""3\5\36:\xF4\xEES\35@\x81\2\20\22r\12\xFF\x8F\xEF\xDFz\0\x85\37""C\4\xF7q\xE7o\xE0"
			"\xF7/\42\x7F\xC9t\xE8)W\x81\x8B\x95\17\xF9m\t\xE2\x8B\xB7\xE4\xFA""b\t\27Gul\xC2\x7F\x88\33""C\xF6""0\xBFkw\xFAV\xC9\4\16\xB5z"
			"MLn.|\x89\xC1\xD3\xBFl \20,\x86\xCB\x91\xE7L\x81q\x8E\xE2\xF4\x9F\xB3^d\xBF^9;\xF6\x97\xBF""d\16""4\xF4\x94\3VM\x80\xFB""1\34"
			"\xE1q\x97\xB4\xE1\xE3\xED""fe'\xD8\xEF""6S\xDA\xC4""c\x94""4Hu\xF4\xEB\xCC\xFE\xDAJ8)6>\x83\x7F\2\xB7\xB5\xB5\x82\xD7\x91\7"
			"\xC1\xBC\xF3\xFByp\16\x80\x99r@k\xE1""AnS5\13[\0\xCC\x8B\xAF""7\x7F\xED{\x98p\xB0:\xD6{Q\x90\xEA\xE8""D\x8F\x84\xFCv\xB4\xBE"
			"\x8F\x99\xB5\xD9\x8Cz\xFC\xA5\xF2]\xDE\x8E!\x98?\xDB_=\xFA\x93S <9\x8A\xBD\36\x8C:\x7F\xEB@\24\xBF""D\xD6""0\31\20""Do\x7F\xE2"
			":\xE5nV\x85\x8B=P\xE0\xECg4\xFE\xD7n\xFF\5\x94\xA7Yo&h\xD5t\0\0\0\0IEND\xAE""B`\x82"
		};

		const CodepointGlyph glyphData[] =
		{
			{ 32, { 52, 80, 3, 3, -1, -1, 3, 0 }}, { 33, { 0, 46, 5, 13, -1, 2, 4, 0 }}, { 34, { 223, 75, 7, 7, -1, 2, 5, 0 }},
			{ 35, { 53, 31, 8, 14, 0, 2, 7, 0 }}, { 36, { 147, 16, 8, 15, -1, 2, 7, 0 }}, { 37, { 229, 15, 13, 14, -1, 2, 12, 0 }},
			{ 38, { 143, 44, 11, 12, -1, 3, 9, 0 }}, { 39, { 251, 75, 5, 7, -1, 2, 3, 0 }}, { 40, { 184, 15, 5, 15, 0, 2, 4, 0 }},
			{ 41, { 199, 15, 5, 15, -1, 2, 4, 0 }}, { 42, { 192, 76, 8, 8, 0, 3, 7, 0 }}, { 43, { 69, 69, 9, 11, 0, 4, 8, 0 }},
			{ 44, { 123, 79, 4, 5, 1, 12, 5, 0 }}, { 45, { 151, 78, 6, 4, 0, 8, 5, 0 }}, { 46, { 39, 82, 3, 3, 1, 12, 4, 0 }},
			{ 47, { 139, 16, 8, 15, -1, 2, 6, 0 }}, { 48, { 117, 56, 9, 12, -1, 3, 7, 0 }}, { 49, { 250, 42, 6, 12, 0, 3, 7, 0 }},
			{ 50, { 214, 42, 9, 12, -1, 3, 7, 0 }}, { 51, { 174, 55, 8, 12, -1, 3, 7, 0 }}, { 52, { 205, 30, 9, 13, 0, 2, 7, 0 }},
			{ 53, { 190, 55, 8, 12, -1, 3, 7, 0 }}, { 54, { 54, 57, 9, 12, -1, 3, 7, 0 }}, { 55, { 142, 56, 8, 12, 0, 3, 7, 0 }},
			{ 56, { 9, 59, 9, 12, -1, 3, 7, 0 }}, { 57, { 0, 59, 9, 12, -1, 3, 7, 0 }}, { 58, { 250, 66, 4, 9, 0, 6, 3, 0 }},
			{ 59, { 29, 70, 5, 12, -1, 6, 3, 0 }}, { 60, { 216, 66, 9, 9, 0, 6, 8, 0 }}, { 61, { 200, 76, 9, 7, 0, 7, 9, 0 }},
			{ 62, { 207, 67, 9, 9, -1, 6, 8, 0 }}, { 63, { 230, 54, 8, 12, -1, 3, 6, 0 }}, { 64, { 84, 31, 14, 13, -1, 4, 13, 0 }},
			{ 65, { 98, 31, 11, 13, -1, 3, 9, 0 }}, { 66, { 134, 56, 8, 12, 0, 3, 8, 0 }}, { 67, { 154, 44, 11, 12, -1, 3, 9, 0 }},
			{ 68, { 121, 44, 11, 12, -1, 3, 10, 0 }}, { 69, { 126, 56, 8, 12, 0, 3, 7, 0 }}, { 70, { 230, 29, 8, 13, 0, 3, 7, 0 }},
			{ 71, { 165, 43, 11, 12, -1, 3, 10, 0 }}, { 72, { 150, 31, 10, 13, 0, 2, 10, 0 }}, { 73, { 44, 69, 3, 12, 0, 3, 3, 0 }},
			{ 74, { 140, 0, 6, 16, -3, 2, 4, 0 }}, { 75, { 27, 58, 9, 12, 0, 3, 8, 0 }}, { 76, { 214, 29, 8, 13, 0, 2, 7, 0 }},
			{ 77, { 48, 45, 13, 12, -1, 3, 13, 0 }}, { 78, { 186, 43, 10, 12, 0, 3, 10, 0 }}, { 79, { 97, 44, 12, 12, -1, 3, 11, 0 }},
			{ 80, { 187, 30, 9, 13, -1, 3, 7, 0 }}, { 81, { 244, 0, 12, 15, -1, 3, 11, 0 }}, { 82, { 140, 31, 10, 13, -1, 3, 8, 0 }},
			{ 83, { 241, 42, 9, 12, -1, 3, 7, 0 }}, { 84, { 160, 30, 9, 13, -1, 3, 7, 0 }}, { 85, { 120, 31, 10, 13, -1, 2, 10, 0 }},
			{ 86, { 109, 31, 11, 13, -1, 2, 9, 0 }}, { 87, { 69, 31, 15, 13, -1, 2, 13, 0 }}, { 88, { 176, 43, 10, 12, -1, 3, 8, 0 }},
			{ 89, { 132, 44, 11, 12, -1, 3, 8, 0 }}, { 90, { 223, 42, 9, 12, 0, 3, 8, 0 }}, { 91, { 189, 15, 5, 15, 0, 2, 5, 0 }},
			{ 92, { 131, 16, 8, 15, -1, 2, 6, 0 }}, { 93, { 194, 15, 5, 15, 0, 2, 5, 0 }}, { 94, { 209, 76, 7, 7, 0, 5, 7, 0 }},
			{ 95, { 43, 81, 9, 3, -1, 13, 8, 0 }}, { 96, { 29, 82, 5, 4, 1, 3, 8, 0 }}, { 97, { 234, 66, 8, 9, -1, 6, 7, 0 }},
			{ 98, { 246, 29, 8, 13, 0, 2, 8, 0 }}, { 99, { 159, 77, 8, 9, -1, 6, 6, 0 }}, { 100, { 169, 30, 9, 13, -1, 2, 8, 0 }},
			{ 101, { 242, 66, 8, 9, -1, 6, 7, 0 }}, { 102, { 37, 31, 8, 14, -1, 2, 5, 0 }}, { 103, { 196, 43, 9, 12, -1, 6, 8, 0 }},
			{ 104, { 45, 31, 8, 14, 0, 2, 8, 0 }}, { 105, { 10, 46, 4, 13, 0, 3, 3, 0 }}, { 106, { 121, 0, 7, 16, -3, 3, 4, 0 }},
			{ 107, { 222, 29, 8, 13, 0, 2, 7, 0 }}, { 108, { 14, 46, 3, 13, 0, 2, 3, 0 }}, { 109, { 124, 68, 12, 10, 0, 6, 12, 0 }},
			{ 110, { 145, 68, 8, 10, 0, 6, 8, 0 }}, { 111, { 198, 67, 9, 9, -1, 6, 8, 0 }}, { 112, { 206, 55, 8, 12, 0, 6, 8, 0 }},
			{ 113, { 18, 58, 9, 12, -1, 6, 8, 0 }}, { 114, { 153, 68, 6, 10, 0, 6, 5, 0 }}, { 115, { 175, 76, 7, 9, -1, 6, 6, 0 }},
			{ 116, { 112, 68, 7, 11, -1, 4, 5, 0 }}, { 117, { 136, 68, 9, 10, -1, 5, 8, 0 }}, { 118, { 225, 66, 9, 9, -1, 6, 7, 0 }},
			{ 119, { 186, 67, 12, 9, -1, 6, 10, 0 }}, { 120, { 167, 77, 8, 9, -1, 6, 6, 0 }}, { 121, { 205, 43, 9, 12, -1, 6, 7, 0 }},
			{ 122, { 182, 76, 7, 9, -1, 6, 6, 0 }}, { 123, { 171, 15, 7, 15, -1, 2, 5, 0 }}, { 124, { 11, 0, 4, 17, 0, 2, 4, 0 }},
			{ 125, { 178, 15, 6, 15, -1, 2, 5, 0 }}, { 126, { 107, 79, 9, 6, 0, 7, 8, 0 }}, { 161, { 5, 46, 5, 13, -1, 6, 4, 0 }},
			{ 163, { 232, 42, 9, 12, -1, 3, 7, 0 }}, { 165, { 196, 30, 9, 13, -1, 2, 7, 0 }}, { 168, { 249, 82, 6, 4, 0, 4, 8, 0 }},
			{ 171, { 216, 75, 7, 7, 0, 7, 7, 0 }}, { 176, { 71, 80, 6, 6, 1, 2, 9, 0 }}, { 180, { 34, 82, 5, 4, 1, 3, 8, 0 }},
			{ 184, { 146, 78, 5, 5, 1, 13, 8, 0 }}, { 187, { 244, 75, 7, 7, 0, 7, 7, 0 }}, { 191, { 182, 55, 8, 12, -1, 6, 6, 0 }},
			{ 192, { 15, 0, 11, 16, -1, 0, 9, 0 }}, { 193, { 59, 0, 11, 16, -1, 0, 9, 0 }}, { 194, { 48, 0, 11, 16, -1, 0, 9, 0 }},
			{ 195, { 26, 0, 11, 16, -1, 0, 9, 0 }}, { 196, { 0, 17, 11, 15, -1, 1, 9, 0 }}, { 197, { 0, 0, 11, 17, -1, -1, 9, 0 }},
			{ 198, { 34, 45, 14, 12, -1, 3, 13, 0 }}, { 199, { 22, 16, 11, 15, -1, 3, 9, 0 }}, { 200, { 123, 16, 8, 15, 0, 0, 7, 0 }},
			{ 201, { 163, 15, 8, 15, 0, 0, 7, 0 }}, { 202, { 155, 15, 8, 15, 0, 0, 7, 0 }}, { 203, { 29, 31, 8, 14, 0, 1, 7, 0 }},
			{ 204, { 146, 0, 5, 16, -2, 0, 3, 0 }}, { 205, { 134, 0, 6, 16, -1, 0, 3, 0 }}, { 206, { 128, 0, 6, 16, -2, 0, 3, 0 }},
			{ 207, { 209, 15, 5, 15, -1, 1, 3, 0 }}, { 209, { 70, 0, 10, 16, 0, 0, 10, 0 }}, { 210, { 196, 0, 12, 15, -1, 0, 11, 0 }},
			{ 211, { 232, 0, 12, 15, -1, 0, 11, 0 }}, { 212, { 220, 0, 12, 15, -1, 0, 11, 0 }}, { 213, { 208, 0, 12, 15, -1, 0, 11, 0 }},
			{ 214, { 242, 15, 12, 14, -1, 1, 11, 0 }}, { 216, { 73, 44, 12, 12, -1, 3, 11, 0 }}, { 217, { 66, 16, 10, 15, -1, 0, 10, 0 }},
			{ 218, { 86, 16, 10, 15, -1, 0, 10, 0 }}, { 219, { 76, 16, 10, 15, -1, 0, 10, 0 }}, { 220, { 11, 32, 9, 14, 0, 1, 10, 0 }},
			{ 221, { 33, 16, 11, 15, -1, 0, 8, 0 }}, { 223, { 178, 30, 9, 13, 0, 2, 9, 0 }}, { 224, { 214, 54, 8, 12, -1, 3, 7, 0 }},
			{ 225, { 246, 54, 8, 12, -1, 3, 7, 0 }}, { 226, { 8, 71, 8, 12, -1, 3, 7, 0 }}, { 227, { 150, 56, 8, 12, -1, 3, 7, 0 }},
			{ 228, { 104, 68, 8, 11, -1, 4, 7, 0 }}, { 229, { 158, 56, 8, 12, -1, 3, 7, 0 }}, { 230, { 173, 67, 13, 9, -1, 6, 11, 0 }},
			{ 231, { 166, 55, 8, 12, -1, 6, 6, 0 }}, { 232, { 0, 71, 8, 12, -1, 3, 7, 0 }}, { 233, { 238, 54, 8, 12, -1, 3, 7, 0 }},
			{ 234, { 222, 54, 8, 12, -1, 3, 7, 0 }}, { 235, { 96, 68, 8, 11, -1, 4, 7, 0 }}, { 236, { 39, 69, 5, 12, -2, 3, 3, 0 }},
			{ 237, { 34, 70, 5, 12, 0, 3, 3, 0 }}, { 238, { 23, 70, 6, 12, -2, 3, 3, 0 }}, { 239, { 119, 68, 5, 11, -1, 4, 3, 0 }},
			{ 241, { 238, 29, 8, 13, 0, 3, 8, 0 }}, { 242, { 45, 57, 9, 12, -1, 3, 8, 0 }}, { 243, { 63, 57, 9, 12, -1, 3, 8, 0 }},
			{ 244, { 72, 57, 9, 12, -1, 3, 8, 0 }}, { 245, { 81, 56, 9, 12, -1, 3, 8, 0 }}, { 246, { 87, 68, 9, 11, -1, 4, 8, 0 }},
			{ 248, { 59, 69, 10, 11, -1, 5, 8, 0 }}, { 249, { 90, 56, 9, 12, -1, 3, 8, 0 }}, { 250, { 99, 56, 9, 12, -1, 3, 8, 0 }},
			{ 251, { 108, 56, 9, 12, -1, 3, 8, 0 }}, { 252, { 78, 69, 9, 11, -1, 4, 8, 0 }}, { 253, { 80, 0, 9, 16, -1, 3, 7, 0 }},
			{ 255, { 96, 16, 9, 15, -1, 4, 7, 0 }}, { 258, { 37, 0, 11, 16, -1, 0, 9, 0 }}, { 259, { 198, 55, 8, 12, -1, 3, 7, 0 }},
			{ 286, { 55, 16, 11, 15, -1, 0, 10, 0 }}, { 287, { 105, 16, 9, 15, -1, 3, 8, 0 }}, { 304, { 204, 15, 5, 15, -1, 0, 4, 0 }},
			{ 305, { 189, 76, 3, 9, 0, 6, 3, 0 }}, { 306, { 107, 0, 7, 16, 0, 2, 7, 0 }}, { 307, { 114, 0, 7, 16, 0, 3, 7, 0 }},
			{ 338, { 17, 46, 17, 12, -1, 3, 15, 0 }}, { 339, { 159, 68, 14, 9, -1, 6, 12, 0 }}, { 350, { 114, 16, 9, 15, -1, 3, 7, 0 }},
			{ 351, { 16, 71, 7, 12, -1, 6, 6, 0 }}, { 372, { 151, 0, 15, 15, -1, 0, 13, 0 }}, { 373, { 109, 44, 12, 12, -1, 3, 10, 0 }},
			{ 374, { 11, 17, 11, 15, -1, 0, 8, 0 }}, { 375, { 98, 0, 9, 16, -1, 3, 7, 0 }}, { 376, { 0, 32, 11, 14, -1, 1, 8, 0 }},
			{ 710, { 23, 82, 6, 4, 0, 3, 8, 0 }}, { 728, { 135, 78, 6, 5, 0, 3, 8, 0 }}, { 730, { 141, 78, 5, 5, 1, 2, 8, 0 }},
			{ 732, { 128, 78, 7, 5, 0, 3, 8, 0 }}, { 7808, { 166, 0, 15, 15, -1, 0, 13, 0 }}, { 7809, { 85, 44, 12, 12, -1, 3, 10, 0 }},
			{ 7810, { 181, 0, 15, 15, -1, 0, 13, 0 }}, { 7811, { 61, 45, 12, 12, -1, 3, 10, 0 }}, { 7812, { 214, 15, 15, 14, -1, 1, 13, 0 }},
			{ 7813, { 47, 69, 12, 11, -1, 4, 10, 0 }}, { 7922, { 44, 16, 11, 15, -1, 0, 8, 0 }}, { 7923, { 89, 0, 9, 16, -1, 3, 7, 0 }},
			{ 8211, { 241, 82, 8, 4, 0, 8, 8, 0 }}, { 8212, { 216, 82, 15, 4, 0, 8, 15, 0 }}, { 8216, { 87, 79, 5, 7, -1, 2, 3, 0 }},
			{ 8217, { 92, 79, 5, 7, -1, 2, 3, 0 }}, { 8220, { 230, 75, 7, 7, -1, 2, 5, 0 }}, { 8221, { 237, 75, 7, 7, -1, 2, 5, 0 }},
			{ 8222, { 116, 79, 7, 6, -1, 11, 6, 0 }}, { 8230, { 231, 82, 10, 4, 0, 11, 10, 0 }}, { 8249, { 97, 79, 5, 7, 0, 7, 4, 0 }},
			{ 8250, { 102, 79, 5, 7, 0, 7, 4, 0 }}, { 8364, { 36, 57, 9, 12, -1, 3, 7, 0 }},
		};

		const Kerning kerningData[] =
		{
			{ 8217, 252, -1 }, { 8217, 251, -1 }, { 8217, 250, -1 }, { 65, 84, -1 }, { 8217, 249, -1 }, { 65, 86, -1 }, { 65, 87, -1 },
			{ 65, 89, -1 }, { 65, 118, -1 }, { 8217, 248, -2 }, { 8217, 246, -2 }, { 8217, 245, -2 }, { 8217, 244, -2 }, { 8217, 243, -2 },
			{ 8217, 242, -2 }, { 8217, 241, -1 }, { 8217, 235, -2 }, { 8217, 234, -2 }, { 8217, 233, -2 }, { 8217, 232, -2 }, { 8217, 231, -2 },
			{ 65, 221, -1 }, { 65, 376, -1 }, { 8217, 229, -1 }, { 8217, 228, -1 }, { 8217, 227, -1 }, { 8217, 226, -1 }, { 8217, 225, -1 },
			{ 8217, 224, -1 }, { 8217, 122, -1 }, { 8217, 120, -1 }, { 76, 84, -2 }, { 76, 86, -1 }, { 76, 87, -1 }, { 8217, 119, -1 },
			{ 8217, 118, -1 }, { 8217, 117, -1 }, { 8217, 116, -1 }, { 8217, 115, -1 }, { 8217, 114, -1 }, { 8217, 113, -2 }, { 79, 88, -1 },
			{ 8217, 111, -2 }, { 8217, 110, -1 }, { 8217, 109, -1 }, { 8217, 103, -2 }, { 8217, 101, -2 }, { 8217, 100, -2 }, { 80, 65, -1 },
			{ 8217, 99, -2 }, { 8217, 97, -1 }, { 376, 248, -1 }, { 80, 192, -1 }, { 80, 193, -1 }, { 80, 194, -1 }, { 80, 195, -1 },
			{ 80, 196, -1 }, { 80, 197, -1 }, { 376, 246, -1 }, { 376, 245, -1 }, { 376, 244, -1 }, { 376, 243, -1 }, { 376, 242, -1 },
			{ 376, 235, -1 }, { 376, 234, -1 }, { 376, 233, -1 }, { 376, 232, -1 }, { 376, 229, -1 }, { 376, 228, -1 }, { 376, 227, -1 },
			{ 376, 226, -1 }, { 376, 225, -1 }, { 376, 224, -1 }, { 376, 197, -1 }, { 376, 196, -1 }, { 376, 195, -1 }, { 376, 194, -1 },
			{ 376, 193, -1 }, { 376, 192, -1 }, { 376, 122, -1 }, { 376, 115, -1 }, { 376, 111, -1 }, { 84, 65, -1 }, { 84, 97, -1 },
			{ 84, 101, -1 }, { 84, 111, -1 }, { 84, 117, -1 }, { 84, 119, -1 }, { 84, 121, -1 }, { 84, 192, -1 }, { 84, 193, -1 },
			{ 84, 194, -1 }, { 84, 195, -1 }, { 84, 196, -1 }, { 84, 197, -1 }, { 376, 101, -1 }, { 376, 97, -1 }, { 376, 65, -1 },
			{ 221, 248, -1 }, { 221, 246, -1 }, { 221, 245, -1 }, { 84, 232, -1 }, { 84, 233, -1 }, { 84, 234, -1 }, { 84, 235, -1 },
			{ 84, 242, -1 }, { 84, 243, -1 }, { 84, 244, -1 }, { 84, 245, -1 }, { 84, 246, -1 }, { 84, 248, -1 }, { 84, 249, -1 },
			{ 84, 250, -1 }, { 84, 251, -1 }, { 84, 252, -1 }, { 221, 244, -1 }, { 221, 243, -1 }, { 84, 339, -1 }, { 221, 242, -1 },
			{ 221, 235, -1 }, { 221, 234, -1 }, { 221, 233, -1 }, { 221, 232, -1 }, { 221, 229, -1 }, { 221, 228, -1 }, { 86, 65, -1 },
			{ 221, 227, -1 }, { 86, 101, -1 }, { 86, 111, -1 }, { 221, 226, -1 }, { 86, 192, -1 }, { 86, 193, -1 }, { 86, 194, -1 },
			{ 86, 195, -1 }, { 86, 196, -1 }, { 86, 197, -1 }, { 221, 225, -1 }, { 221, 224, -1 }, { 221, 197, -1 }, { 221, 196, -1 },
			{ 221, 195, -1 }, { 221, 194, -1 }, { 86, 232, -1 }, { 86, 233, -1 }, { 86, 234, -1 }, { 86, 235, -1 }, { 86, 242, -1 },
			{ 86, 243, -1 }, { 86, 244, -1 }, { 86, 245, -1 }, { 86, 246, -1 }, { 86, 248, -1 }, { 221, 193, -1 }, { 221, 192, -1 },
			{ 221, 122, -1 }, { 221, 115, -1 }, { 86, 339, -1 }, { 87, 65, -1 }, { 221, 111, -1 }, { 221, 101, -1 }, { 221, 97, -1 },
			{ 221, 65, -1 }, { 87, 192, -1 }, { 87, 193, -1 }, { 87, 194, -1 }, { 87, 195, -1 }, { 87, 196, -1 }, { 87, 197, -1 },
			{ 216, 88, -1 }, { 214, 88, -1 }, { 213, 88, -1 }, { 212, 88, -1 }, { 211, 88, -1 }, { 210, 88, -1 }, { 197, 376, -1 },
			{ 197, 221, -1 }, { 197, 118, -1 }, { 197, 89, -1 }, { 197, 87, -1 }, { 197, 86, -1 }, { 197, 84, -1 }, { 196, 376, -1 },
			{ 196, 221, -1 }, { 196, 118, -1 }, { 196, 89, -1 }, { 196, 87, -1 }, { 196, 86, -1 }, { 196, 84, -1 }, { 195, 376, -1 },
			{ 195, 221, -1 }, { 195, 118, -1 }, { 195, 89, -1 }, { 195, 87, -1 }, { 195, 86, -1 }, { 195, 84, -1 }, { 89, 65, -1 },
			{ 89, 97, -1 }, { 89, 101, -1 }, { 89, 111, -1 }, { 89, 115, -1 }, { 194, 376, -1 }, { 89, 122, -1 }, { 89, 192, -1 },
			{ 89, 193, -1 }, { 89, 194, -1 }, { 89, 195, -1 }, { 89, 196, -1 }, { 89, 197, -1 }, { 89, 224, -1 }, { 89, 225, -1 },
			{ 89, 226, -1 }, { 89, 227, -1 }, { 89, 228, -1 }, { 89, 229, -1 }, { 89, 232, -1 }, { 89, 233, -1 }, { 89, 234, -1 },
			{ 89, 235, -1 }, { 89, 242, -1 }, { 89, 243, -1 }, { 89, 244, -1 }, { 89, 245, -1 }, { 89, 246, -1 }, { 89, 248, -1 },
			{ 194, 221, -1 }, { 194, 118, -1 }, { 194, 89, -1 }, { 194, 87, -1 }, { 194, 86, -1 }, { 194, 84, -1 }, { 193, 376, -1 },
			{ 193, 221, -1 }, { 193, 118, -1 }, { 193, 89, -1 }, { 193, 87, -1 }, { 193, 86, -1 }, { 192, 84, -1 }, { 193, 84, -1 },
			{ 192, 86, -1 }, { 192, 87, -1 }, { 192, 89, -1 }, { 192, 118, -1 }, { 192, 376, -1 }, { 192, 221, -1 },
		};

		PrebakedFontData out;
		out.image.LoadFromMemory(fileData, std::size(fileData));
		out.fontType = FontType::Bitmap;
		out.errorGlyph = { 130, 31, 10, 13, -1, 2, 8, 0 };
		out.fontDesc = { "Vegur15", 15, 14, 18, 0 };

		out.glyphs.resize(std::size(glyphData));
		memcpy(out.glyphs.data(), glyphData, std::size(glyphData) * sizeof(CodepointGlyph));

		out.kerns.resize(std::size(kerningData));
		memcpy(out.kerns.data(), kerningData, std::size(kerningData) * sizeof(Kerning));

		return out;
	}


	const uint16_t PrebakedFontFile_MajorVersion = 1;
	const uint16_t PrebakedFontFile_MinorVersion = 0;
	struct PrebakedFontFileHeader
	{
		char fileDescriptor[8] = {}; // IGLOFONT
		uint16_t majorVersion = {};
		uint16_t minorVersion = {};
		uint64_t numCodepointGlyphs = {};
		uint64_t numKerns = {};
		uint32_t fontType = {};

		uint64_t fontNameStrSize = {};
		float fontSize = {};
		int32_t baseline = {};
		int32_t lineHeight = {};
		int32_t lineGap = {};

		Glyph errorGlyph;

		uint32_t imageWidth = {};
		uint32_t imageHeight = {};
		uint32_t imageFormat = {};
		uint32_t imageMipLevels = {};
		uint32_t imageNumFaces = {};
		uint32_t imageArrangement = {};


		uint64_t GetGlyphsByteSize()
		{
			return numCodepointGlyphs * sizeof(CodepointGlyph);
		}
		static uint64_t GetSingleKernSize()
		{
			return sizeof(uint32_t) + sizeof(uint32_t) + sizeof(int16_t);
		}
		uint64_t GetKernsByteSize()
		{
			return numKerns * (GetSingleKernSize());
		}
		uint64_t GetTotalFileSize()
		{
			uint64_t totalSize = sizeof(PrebakedFontFileHeader);
			totalSize += fontNameStrSize;
			totalSize += GetGlyphsByteSize();
			totalSize += GetKernsByteSize();
			totalSize += Image::CalculateTotalSize(imageMipLevels, imageNumFaces, imageWidth, imageHeight, (Format)imageFormat);
			return totalSize;
		}
	};

	bool PrebakedFontData::SaveToFile(const std::string& filename)
	{
		if (!image.IsLoaded())
		{
			Log(LogType::Error, "Failed to save prebaked font to file '" + filename +
				"'. Reason: The provided image isn't loaded with any data.");
			return false;
		}

		PrebakedFontFileHeader header;
		std::memcpy(header.fileDescriptor, "IGLOFONT", 8);
		header.majorVersion = PrebakedFontFile_MajorVersion;
		header.minorVersion = PrebakedFontFile_MinorVersion;
		header.numCodepointGlyphs = (uint64_t)glyphs.size();
		header.numKerns = (uint64_t)kerns.size();
		header.fontType = (uint32_t)fontType;
		header.fontNameStrSize = (uint64_t)fontDesc.fontName.size();
		header.fontSize = fontDesc.fontSize;
		header.baseline = fontDesc.baseline;
		header.lineHeight = fontDesc.lineHeight;
		header.lineGap = fontDesc.lineGap;
		header.errorGlyph = errorGlyph;

		header.imageWidth = image.GetWidth();
		header.imageHeight = image.GetHeight();
		header.imageFormat = (uint32_t)image.GetFormat();
		header.imageMipLevels = image.GetNumMipLevels();
		header.imageNumFaces = image.GetNumFaces();
		header.imageArrangement = (uint32_t)image.GetArrangement();

		std::ofstream outFile(std::filesystem::u8path(filename), std::ios::out | std::ios::binary);
		if (!outFile)
		{
			Log(LogType::Error, "Failed to save prebaked font to file '" + filename + "'. Reason: Couldn't open file.");
			return false;
		}

		outFile.write((char*)&header, sizeof(PrebakedFontFileHeader));
		outFile.write((char*)fontDesc.fontName.c_str(), header.fontNameStrSize);
		outFile.write((char*)glyphs.data(), header.GetGlyphsByteSize());
		for (size_t i = 0; i < header.numKerns; i++)
		{
			outFile.write((char*)&kerns[i].codepointPrev, sizeof(uint32_t));
			outFile.write((char*)&kerns[i].codepointNext, sizeof(uint32_t));
			outFile.write((char*)&kerns[i].x, sizeof(int16_t));
		}
		outFile.write((char*)image.GetPixels(), image.GetSize());
		return true;
	}

	bool PrebakedFontData::LoadFromFile(const std::string& filename)
	{
		ReadFileResult file = ReadFile(filename);
		if (!file.success)
		{
			Log(LogType::Error, "Failed to load prebaked font from file '" + filename + "'. Reason: Unable to open file.");
			return false;
		}
		return LoadFromMemory(file.fileContent.data(), file.fileContent.size());
	}

	bool PrebakedFontData::LoadFromMemory(const byte* fileData, size_t numBytes)
	{
		const char* errStr = "Failed to load prebaked font. Reason: ";

		PrebakedFontFileHeader* header = (PrebakedFontFileHeader*)fileData;

		if (numBytes == 0 || fileData == nullptr)
		{
			Log(LogType::Error, ToString(errStr, "No file data provided."));
			return false;
		}

		if (numBytes < sizeof(PrebakedFontFileHeader))
		{
			Log(LogType::Error, ToString(errStr, "File is corrupt (it is smaller than expected)."));
			return false;
		}

		std::string fileDescriptor(header->fileDescriptor, 8);
		if (fileDescriptor != "IGLOFONT")
		{
			Log(LogType::Error, ToString(errStr, "Not an IGLOFONT file."));
			return false;
		}

		bool isCorrectVersion = (
			header->majorVersion == PrebakedFontFile_MajorVersion &&
			header->minorVersion == PrebakedFontFile_MinorVersion);
		if (!isCorrectVersion)
		{
			Log(LogType::Error, ToString(errStr, "File uses a newer or older version of the file format."));
			return false;
		}

		// Make sure file is not smaller than expected
		if (numBytes < header->GetTotalFileSize())
		{
			Log(LogType::Error, ToString(errStr, "File is corrupt (it is smaller than expected)."));
			return false;
		}

		// Larger than expected
		if (numBytes > header->GetTotalFileSize())
		{
			Log(LogType::Warning, "Prebaked font file was larger than expected.");
		}

		this->fontType = (FontType)header->fontType;
		this->fontDesc.fontSize = header->fontSize;
		this->fontDesc.baseline = header->baseline;
		this->fontDesc.lineHeight = header->lineHeight;
		this->fontDesc.lineGap = header->lineGap;
		this->errorGlyph = header->errorGlyph;

		const byte* src = fileData;
		src += sizeof(PrebakedFontFileHeader);

		this->fontDesc.fontName.clear();
		this->fontDesc.fontName.resize((size_t)header->fontNameStrSize);
		memcpy(this->fontDesc.fontName.data(), src, header->fontNameStrSize);
		src += header->fontNameStrSize;

		this->glyphs.clear();
		this->glyphs.resize((size_t)header->numCodepointGlyphs);
		memcpy(this->glyphs.data(), src, header->GetGlyphsByteSize());
		src += header->GetGlyphsByteSize();

		this->kerns.clear();
		this->kerns.resize((size_t)header->numKerns);
		for (size_t i = 0; i < this->kerns.size(); i++)
		{
			memcpy(&this->kerns[i], src, PrebakedFontFileHeader::GetSingleKernSize());
			src += PrebakedFontFileHeader::GetSingleKernSize();
		}

		if (!this->image.Load(header->imageWidth, header->imageHeight, (Format)header->imageFormat, header->imageMipLevels,
			header->imageNumFaces, false, (Image::Arrangement)header->imageArrangement))
		{
			Log(LogType::Error, ToString(errStr, "Failed to load font image."));
			return false;
		}

		memcpy(this->image.GetPixels(), src, this->image.GetSize());

		return true;
	}

	bool Font::LoadAsPrebakedFromFile(const IGLOContext& context, CommandList& cmd, const std::string& filename)
	{
		Unload();
		PrebakedFontData prebaked;
		if (!prebaked.LoadFromFile(filename)) return false;
		return LoadAsPrebaked(context, cmd, prebaked);
	}

	bool Font::LoadAsPrebakedFromMemory(const IGLOContext& context, CommandList& cmd, const byte* fileData, size_t numBytes)
	{
		Unload();
		PrebakedFontData prebaked;
		if (!prebaked.LoadFromMemory(fileData, numBytes)) return false;
		return LoadAsPrebaked(context, cmd, prebaked);
	}

	bool Font::LoadAsPrebaked(const IGLOContext& context, CommandList& cmd, const PrebakedFontData& data)
	{
		Unload();
		if (data.fontDesc.fontSize < 1)
		{
			Log(LogType::Error, "Failed to load prebaked font '" + data.fontDesc.fontName +
				"'. Reason: Font size smaller than 1 not allowed.");
			Unload();
			return false;
		}

		this->isLoaded = true;
		this->context = &context;
		this->fontDesc = data.fontDesc;
		this->fileDataReadOnly = nullptr;
		this->isPrebaked = true;

		FontSettings temp = FontSettings();
		temp.fontType = data.fontType;
		this->fontSettings = temp;

		SetPrebakedGlyphs(data.glyphs, data.errorGlyph);
		SetKerning(data.kerns);

		this->page.texture = std::make_shared<Texture>();
		if (!this->page.texture->LoadFromMemory(context, cmd, data.image, false))
		{
			Log(LogType::Error, "Failed to load prebaked font '" + data.fontDesc.fontName +
				"'. Reason: Failed to create a texture from the provided image.");
			Unload();
			return false;
		}
		return true;
	}

	bool Font::LoadFromFile(const IGLOContext& context, const std::string& filename, float fontSize, FontSettings fontSettings)
	{
		Unload();
		ReadFileResult file = ReadFile(filename);
		if (!file.success)
		{
			Log(LogType::Error, "Failed to load font from file. Reason: Couldn't open '" + filename + "'.");
			return false;
		}
		if (file.fileContent.size() == 0)
		{
			Log(LogType::Error, "Failed to load font from file. Reason: File '" + filename + "' is empty.");
			return false;
		}
		if (LoadFromMemory(context, file.fileContent.data(), file.fileContent.size(), filename, fontSize, fontSettings))
		{
			this->fileDataOwned.swap(file.fileContent); // This font owns the file data buffer.
			return true; // Success
		}
		else
		{
			Unload();
			return false;
		}
	}

	bool Font::LoadFromMemory(const IGLOContext& context, const byte* data, size_t numBytes, std::string fontName, float fontSize,
		FontSettings fontSettings)
	{
		Unload();
		if (data == nullptr || numBytes == 0)
		{
			Log(LogType::Error, "Failed to load font '" + fontName + "'. Reason: No file data provided.");
			Unload();
			return false;
		}
		if (fontSize < 1)
		{
			Log(LogType::Error, "Failed to load font '" + fontName + "'. Reason: Font size smaller than 1 not allowed.");
			Unload();
			return false;
		}
		if (fontSettings.fontType == FontType::SDF && (int16_t)fontSettings.sdfOutwardGradientSize < 0)
		{
			Log(LogType::Error, "Failed to load SDF font '" + fontName + "'. Reason: sdfOutwardGradientSize is too large.");
			Unload();
			return false;
		}

		// Init font
		if (!stbtt_InitFont(&this->stbttFontInfo, data, 0))
		{
			Log(LogType::Error, "Failed to load font '" + fontName + "'. File might be corrupted or unsupported by stb_truetype.");
			Unload();
			return false;
		}

		this->isLoaded = true;
		this->context = &context;
		this->isPrebaked = false;
		this->fontDesc.fontName = fontName;
		this->fontDesc.fontSize = fontSize;
		this->fontSettings = fontSettings;
		this->fileDataReadOnly = data;

		// Get font metrics
		this->stbttScale = stbtt_ScaleForMappingEmToPixels(&this->stbttFontInfo, fontSize);
		float scaleForPixelHeight = stbtt_ScaleForPixelHeight(&this->stbttFontInfo, fontSize);
		this->fontDesc.lineHeight = int(ceilf(fontSize * (this->stbttScale / scaleForPixelHeight)));
		int descender = 0;
		stbtt_GetFontVMetrics(&this->stbttFontInfo, &this->fontDesc.baseline, &descender, &this->fontDesc.lineGap);
		this->fontDesc.baseline = int(ceilf(((float)this->fontDesc.baseline) * this->stbttScale));
		this->fontDesc.lineGap = int(ceilf(((float)this->fontDesc.lineGap) * this->stbttScale));

		// Extract glyph and kerning info so we can map them to codepoints
		int kernTableLength = stbtt_GetKerningTableLength(&this->stbttFontInfo);
		std::vector<stbtt_kerningentry> kernEntry;
		kernEntry.resize(kernTableLength);
		int result = stbtt_GetKerningTable(&this->stbttFontInfo, kernEntry.data(), kernTableLength);
		std::vector<uint32_t> tableCodepoints;
		PackedBoolArray exists; // One bool value for each element in 'tableCodepoints', indicating whether it exists.
		tableCodepoints.resize((size_t)this->stbttFontInfo.numGlyphs + 1);
		exists.Resize(tableCodepoints.size());
		uint32_t totalNumValidCodepoints = 0;
		for (uint32_t codepoint = 0; codepoint < 0x10FFFF; codepoint++)
		{
			int index = stbtt_FindGlyphIndex(&this->stbttFontInfo, codepoint);
			if (index != 0)
			{
				// Multiple codepoints can point to the same glyph indexes.
				totalNumValidCodepoints++;
				tableCodepoints[index] = codepoint;
				exists.SetTrue(index);
			}
		}
		std::vector<Kerning> kerning;
		kerning.resize(kernEntry.size());
		for (size_t i = 0; i < kernEntry.size(); i++)
		{
			kerning[i].codepointPrev = tableCodepoints[kernEntry[i].glyph1];
			kerning[i].codepointNext = tableCodepoints[kernEntry[i].glyph2];
			kerning[i].x = int16_t(roundf(float(kernEntry[i].advance) * this->stbttScale));
		}


		SetDynamicGlyphs(tableCodepoints, exists, totalNumValidCodepoints);
		SetKerning(kerning);

		return true;
	}

	inline uint64_t Font::GetKernKey(uint32_t codepointPrev, uint32_t codepointNext) const
	{
		uint64_t combined = (uint64_t)codepointPrev << 32;
		combined = combined + (uint64_t)codepointNext;
		return combined;
	}

	void Font::SetKerning(const std::vector<Kerning>& kernList)
	{
		atlas.highestLeftKernCodepoint = 0;
		atlas.kernCount = 0;
		for (uint32_t i = 0; i < kernList.size(); i++)
		{
			// If there is no kerning
			if (kernList[i].x == 0) continue;

			atlas.kernCount++; // Kerning counted

			if (kernList[i].codepointPrev > atlas.highestLeftKernCodepoint)
			{
				// Highest left kern codepoint found so far
				atlas.highestLeftKernCodepoint = kernList[i].codepointPrev;
			}
			if (kernList[i].codepointPrev < atlas.glyphTableSize)
			{
				// Glyph on left side of this kerning should be marked as belonging to atleast one kerning pair (on left side).
				atlas.glyphTable[kernList[i].codepointPrev].flags |= (uint16_t)Atlas::GlyphFlags::HasLeftKern;
			}
			atlas.kernMap.emplace(GetKernKey(kernList[i].codepointPrev, kernList[i].codepointNext), kernList[i].x);
		}
	}

	void Font::SetDynamicGlyphs(const std::vector<uint32_t>& codepoints, const PackedBoolArray& exists, uint32_t totalNumValidCodepoints)
	{
		// Some fonts have multiple codepoints bound to the same glyph index,
		// which results in a higher number of valid codepoints than there are glyphs in the font.
		// glyphCount is set to number of valid codepoints instead of number of valid glyphs,
		// because loadedGlyphCount increases by 1 for every new codepoint loaded.
		// This way, loadedGlyphCount will never go above glyphCount.
		atlas.glyphCount = totalNumValidCodepoints;
		atlas.highestGlyphCodepoint = 0;
		atlas.loadedGlyphCount = 0;
		atlas.glyphMap.clear();

		atlas.glyphTable.clear();
		atlas.glyphTable.shrink_to_fit();
		atlas.glyphTable.resize(atlas.glyphTableSize);

		for (size_t i = 0; i < codepoints.size(); i++)
		{
			// If this codepoint does not belong to any glyph
			if (!exists.GetAt(i)) continue;

			if (codepoints[i] > atlas.highestGlyphCodepoint)
			{
				// Highest codepoint found so far
				atlas.highestGlyphCodepoint = codepoints[i];
			}
		}

		// Codepoint 0xffff is guaranteed to result in an error glyph.
		atlas.errorGlyph = LoadGlyph(0xffff);
	}

	void Font::SetPrebakedGlyphs(const std::vector<CodepointGlyph>& codepointGlyphs, Glyph errorGlyph)
	{
		atlas.glyphCount = 0;
		atlas.highestGlyphCodepoint = 0;
		atlas.loadedGlyphCount = 0;
		atlas.glyphMap.clear();

		atlas.glyphTable.clear();
		atlas.glyphTable.shrink_to_fit();
		atlas.glyphTable.resize(atlas.glyphTableSize);

		for (size_t i = 0; i < codepointGlyphs.size(); i++)
		{
			atlas.glyphCount++; // Glyph counted
			atlas.loadedGlyphCount++; // Loaded glyph counted
			if (codepointGlyphs[i].codepoint > atlas.highestGlyphCodepoint)
			{
				// Set the highest codepoint found so far
				atlas.highestGlyphCodepoint = codepointGlyphs[i].codepoint;
			}
			if (codepointGlyphs[i].codepoint < atlas.glyphTableSize)
			{
				// Put this glyph in the glyph table, which flag saying it is loaded.
				atlas.glyphTable[codepointGlyphs[i].codepoint] = codepointGlyphs[i].glyph;
				atlas.glyphTable[codepointGlyphs[i].codepoint].flags = (uint16_t)Atlas::GlyphFlags::IsLoaded;
			}
			else
			{
				// Put this glyph in the glyph map, flag not needed
				atlas.glyphMap[codepointGlyphs[i].codepoint] = codepointGlyphs[i].glyph;
			}
		}

		atlas.errorGlyph = errorGlyph;
	}

	void Font::Unload()
	{
		isLoaded = false;
		context = nullptr;
		isPrebaked = false;

		fileDataReadOnly = nullptr;
		fileDataOwned.clear();
		fileDataOwned.shrink_to_fit();

		fontDesc = FontDesc();
		fontSettings = FontSettings();

		stbttScale = 0;
		stbttFontInfo = stbtt_fontinfo();

		atlas = Atlas();

		page.texture = nullptr;
		page.rects.clear();
		page.rects.shrink_to_fit();
		page.pixels.clear();
		page.pixels.shrink_to_fit();
		page.width = 0;
		page.height = 0;
		page.dirty = false;

	}

	void Font::ClearTexture()
	{
		if (!isLoaded) return;
		if (IsPrebaked()) return;

		atlas.loadedGlyphCount = 0;
		for (size_t i = 0; i < atlas.glyphTableSize; i++)
		{
			// Unset the IsLoaded flag for all glyphs in the table
			atlas.glyphTable[i].flags &= ~(uint16_t)Atlas::GlyphFlags::IsLoaded;
		}
		atlas.glyphMap.clear();

		if (page.texture) context->DelayedTextureUnload(page.texture);
		page.texture = nullptr;
		page.rects.clear();
		page.rects.shrink_to_fit();
		page.pixels.clear();
		page.pixels.shrink_to_fit();
		page.width = 0;
		page.height = 0;
		page.dirty = false;

		// Codepoint 0xffff is guaranteed to result in an error glyph.
		atlas.errorGlyph = LoadGlyph(0xffff);
	}

	void Font::PreloadGlyphs(uint32_t first, uint32_t last)
	{
		if (!isLoaded) return;
		if (IsPrebaked()) return;

		// As an optimization, cap to highest valid codepoint.
		uint32_t cappedLast = last;
		if (cappedLast > atlas.highestGlyphCodepoint) cappedLast = atlas.highestGlyphCodepoint;

		for (uint64_t i = first; i <= (uint64_t)cappedLast; i++)
		{
			GetGlyph((uint32_t)i);
		}
	}

	void Font::PreloadGlyphs(const std::string& utf8string)
	{
		if (!isLoaded) return;
		if (IsPrebaked()) return;
		std::u32string codepoints = utf8_to_utf32(utf8string);
		for (size_t i = 0; i < codepoints.size(); i++)
		{
			GetGlyph(codepoints[i]);
		}
	}

	bool Font::HasGlyph(uint32_t codepoint) const
	{
		if (!isLoaded) return false;
		if (codepoint < atlas.glyphTableSize)
		{
			uint16_t flag = (uint16_t)Atlas::GlyphFlags::IsLoaded;
			if ((atlas.glyphTable[codepoint].flags & flag) == flag)
			{
				return true; // Glyph is loaded according to its flag value
			}
		}
		else
		{
			auto pos = atlas.glyphMap.find(codepoint);
			if (pos != atlas.glyphMap.end())
			{
				return true; // Glyph is loaded because it exists in the glyph map
			}
		}

		// Glyph isn't loaded. If prebaked, can't load new glyphs, so glyph can't exist.
		if (isPrebaked) return false;

		// If glyph exists in font file
		int stbttGlyphIndex = stbtt_FindGlyphIndex(&stbttFontInfo, codepoint);
		if (stbttGlyphIndex != 0) return true;

		return false;
	}

	void Font::ApplyChangesToTexture(CommandList& cmd)
	{
		if (!isLoaded) return;
		if (!page.dirty) return;
		if (isPrebaked) return;

		page.dirty = false;

		if (page.width == 0 || page.height == 0)
		{
			if (page.texture) context->DelayedTextureUnload(page.texture);
			page.texture = nullptr;
			return;
		}

		// If dimensions have changed, must load new texture
		bool mustLoadNewTexture = true;
		if (page.texture)
		{
			if (page.texture->IsLoaded())
			{
				if (page.texture->GetWidth() == page.width && page.texture->GetHeight() == page.height)
				{
					mustLoadNewTexture = false;
				}
			}
		}

		if (mustLoadNewTexture)
		{
			// We can't instantly unload the previous texture because the GPU might still depend on it in a previous frame.
			// We let IGLOContext hold a shared pointer to the old texture so it remains loaded for a while longer.
			if (page.texture) context->DelayedTextureUnload(page.texture);
			page.texture = std::make_shared<Texture>();

			// The code below expects the texture to already have a certain barrier layout
			BarrierLayout initLayout = BarrierLayout::_GraphicsQueue_ShaderResource;

			if (!page.texture->Load(*context, page.width, page.height, Format::BYTE, TextureUsage::Default,
				MSAA::Disabled, 1, 1, false, ClearValue(), &initLayout))
			{
				Log(LogType::Error, ToString("Failed to apply changes to font texture."
					" Reason: Failed to create texture with size ", page.width, "x", page.height, "."));
				page.texture = nullptr;
				return;
			}
		}

		// Update texture
		cmd.AddTextureBarrier(*page.texture, SimpleBarrier::PixelShaderResource, SimpleBarrier::CopyDest);
		cmd.FlushBarriers();

		page.texture->SetPixels(cmd, page.pixels.data());

		cmd.AddTextureBarrier(*page.texture, SimpleBarrier::CopyDest, SimpleBarrier::PixelShaderResource);
		cmd.FlushBarriers();
	}

	int16_t Font::GetKerning(uint32_t prev, uint32_t next) const
	{
		// If codepoint is low enough, we can optimize by checking if a kerning pair exists before using the map
		if (prev < atlas.glyphTableSize)
		{
			uint16_t flag = (uint16_t)Atlas::GlyphFlags::HasLeftKern;
			bool hasKern = ((atlas.glyphTable[prev].flags & flag) == flag);
			if (!hasKern) return 0;
		}

		// If codepoint is too high
		if (prev > atlas.highestLeftKernCodepoint) return 0;

		auto pos = atlas.kernMap.find(GetKernKey(prev, next));
		if (pos == atlas.kernMap.end())
		{
			return 0;
		}
		else
		{
			uint16_t value = pos->second;
			return value;
		}
	}

	Glyph Font::GetGlyph(uint32_t codepoint)
	{
		// If codepoint is too high
		if (codepoint > atlas.highestGlyphCodepoint)
		{
			return atlas.errorGlyph;
		}

		// If codepoint is low enough, use the fast glyph table
		if (codepoint < atlas.glyphTableSize)
		{
			// If this glyph is loaded
			uint16_t flag = (uint16_t)Atlas::GlyphFlags::IsLoaded;
			if ((atlas.glyphTable[codepoint].flags & flag) == flag)
			{
				return atlas.glyphTable[codepoint];
			}
		}
		else
		{
			// Use the glyph map
			auto pos = atlas.glyphMap.find(codepoint);
			if (pos != atlas.glyphMap.end())
			{
				return pos->second;
			}
		}

		if (isPrebaked)
		{
			// If font is prebaked, can't load new glyphs, so return error glyph
			return atlas.errorGlyph;
		}
		else
		{
			// If font is dynamic, load new glyph

			// If for some weird reason there is no pointer to file data, return error glyph
			if (!fileDataReadOnly)
			{
				Log(LogType::Warning, "Failed to load new glyph for font '" + fontDesc.fontName +
					"'. Reason: Font doesn't have a font file to read from. This should be impossible.");
				return atlas.errorGlyph;
			}

			int stbttGlyphIndex = stbtt_FindGlyphIndex(&stbttFontInfo, codepoint);

			// If font does not contain glyph
			if (stbttGlyphIndex == 0)
			{
				return atlas.errorGlyph;
			}

			Glyph out = LoadGlyph(codepoint);
			atlas.loadedGlyphCount++;

			if (codepoint < atlas.glyphTableSize)
			{
				// The glyph in the glyph table may contain info in its flag about whether it has kerning or not.
				// Make sure this flag does not get overwritten.
				uint16_t oldFlag = atlas.glyphTable[codepoint].flags;
				atlas.glyphTable[codepoint] = out;
				atlas.glyphTable[codepoint].flags = oldFlag;
				// Mark this glyph as being loaded
				atlas.glyphTable[codepoint].flags |= (uint16_t)Atlas::GlyphFlags::IsLoaded;
			}
			else
			{
				atlas.glyphMap[codepoint] = out;
			}
			return out;
		}
	}

	Glyph Font::LoadGlyph(uint32_t codepoint)
	{
		Glyph out;

		if (!isLoaded) return out;
		if (isPrebaked) return out;

		int stbttGlyphIndex = stbtt_FindGlyphIndex(&stbttFontInfo, codepoint);
		int advX = 0;
		stbtt_GetGlyphHMetrics(&stbttFontInfo, stbttGlyphIndex, &advX, 0);
		out.advanceX = int(roundf(float(advX) * stbttScale));

		if (fontSettings.fontType == FontType::SDF) // Signed distance field
		{
			unsigned char onEdgeValue = 128;
			float pixelDistScale = float(onEdgeValue) / float(fontSettings.sdfOutwardGradientSize);
			int padding = fontSettings.sdfOutwardGradientSize;

			// Rasterize and get glyph metrics
			int gw = 0, gh = 0, gofx = 0, gofy = 0;
			unsigned char* bitmap = stbtt_GetGlyphSDF(&stbttFontInfo, stbttScale, stbttGlyphIndex, padding, onEdgeValue, pixelDistScale,
				&gw, &gh, &gofx, &gofy);

			// Failed to get bitmap or there is no bitmap
			if (bitmap == nullptr) return out;

			out.glyphWidth = gw;
			out.glyphHeight = gh;
			out.glyphOffsetX = gofx;
			out.glyphOffsetY = fontDesc.baseline + gofy;

			if (out.glyphWidth == 0 || out.glyphHeight == 0) // No glyph size
			{
				stbtt_FreeSDF(bitmap, nullptr);
				return out;
			}

			// Padding is added to the glyph rect
			IntRect rect = AllocateRoomForGlyph(
				out.glyphWidth + (fontSettings.glyphPadding * 2),
				out.glyphHeight + (fontSettings.glyphPadding * 2));
			if (rect == IntRect(0, 0, 0, 0)) // No room found
			{
				stbtt_FreeSDF(bitmap, nullptr);
				return out;
			}

			// Place glyph
			PlaceGlyph(bitmap,
				rect.left + fontSettings.glyphPadding,
				rect.top + fontSettings.glyphPadding,
				out.glyphWidth,
				out.glyphHeight);

			out.glyphOffsetX -= fontSettings.glyphPadding;
			out.glyphOffsetY -= fontSettings.glyphPadding;
			out.texturePosX = rect.left;
			out.texturePosY = rect.top;
			out.glyphWidth = rect.GetWidth();
			out.glyphHeight = rect.GetHeight();
			stbtt_FreeSDF(bitmap, nullptr);
			return out;
		}
		else if (fontSettings.fontType == FontType::Bitmap)
		{
			// Get glyph metrics
			int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
			float shiftX = 0;
			float shiftY = 0;
			stbtt_GetGlyphBitmapBoxSubpixel(&stbttFontInfo, stbttGlyphIndex, stbttScale, stbttScale, shiftX, shiftY, &x1, &y1, &x2, &y2);
			out.glyphWidth = uint16_t(abs(x2 - x1));
			out.glyphHeight = uint16_t(abs(y2 - y1));
			out.glyphOffsetY = fontDesc.baseline + y1;
			out.glyphOffsetX = x1;

			if (out.glyphWidth == 0 || out.glyphHeight == 0) // No glyph size
			{
				return out;
			}

			// Rasterize
			int stride = (x2 - x1);
			std::vector<byte> bitmap;
			bitmap.resize(((size_t)x2 - x1) * ((size_t)y2 - y1));
			stbtt_MakeGlyphBitmapSubpixel(&stbttFontInfo, bitmap.data(), out.glyphWidth, out.glyphHeight,
				stride, stbttScale, stbttScale, shiftX, shiftY, stbttGlyphIndex);

			// Padding is added to the glyph rect
			IntRect rect = AllocateRoomForGlyph(
				out.glyphWidth + (fontSettings.glyphPadding * 2),
				out.glyphHeight + (fontSettings.glyphPadding * 2));
			if (rect == IntRect(0, 0, 0, 0)) // No room found
			{
				return out;
			}

			// Place glyph
			PlaceGlyph(bitmap.data(),
				rect.left + fontSettings.glyphPadding,
				rect.top + fontSettings.glyphPadding,
				out.glyphWidth,
				out.glyphHeight);

			out.glyphOffsetX -= fontSettings.glyphPadding;
			out.glyphOffsetY -= fontSettings.glyphPadding;
			out.texturePosX = rect.left;
			out.texturePosY = rect.top;
			out.glyphWidth = rect.GetWidth();
			out.glyphHeight = rect.GetHeight();
			return out;
		}
		else
		{
			return out;
		}
	}

	IntRect Font::AllocateRoomForGlyph(uint16_t glyphWidth, uint16_t glyphHeight)
	{
		if (glyphWidth == 0 || glyphHeight == 0) return IntRect(0, 0, 0, 0);

		// If pixels are empty, set to starting size
		if (page.width == 0 || page.height == 0)
		{
			uint32_t startSize = fontSettings.startingTextureSize;
			if (startSize == 0) startSize = 1;
			page.width = startSize;
			page.height = startSize;
			page.pixels.clear();
			page.pixels.shrink_to_fit();
			page.pixels.resize((size_t)page.width * page.height);
			page.dirty = true;
		}

		bool foundMatch = false;
		size_t bestIndex = 0;
		float bestRatio = 0;
		IntRect bestRect = IntRect(0, 0, 0, 0);
		const float maxRatio = 1.3f; // Acceptable height ratio between rect and glyph

		// Search for a rectangle that matches the height of this glyph and isn't already full
		for (size_t i = 0; i < page.rects.size(); i++)
		{
			float ratio = (float)page.rects[i].GetHeight() / (float)glyphHeight;
			if (ratio >= 1 && ratio <= maxRatio) // Check height
			{
				if (ratio <= bestRatio) continue;
				if ((int64_t)page.rects[i].GetWidth() + glyphWidth <= (int64_t)page.width) // Check width
				{
					foundMatch = true;
					bestIndex = i;
					bestRect = page.rects[i];
					bestRatio = ratio;
				}
			}
		}
		// If no rectangle with matching height was found, create new one
		if (!foundMatch)
		{
			int top = 0;
			if (page.rects.size() > 0) top = page.rects[page.rects.size() - 1].bottom;
			IntRect rect;
			rect.left = 0;
			rect.right = 0; // Start with a width of 0 
			rect.top = top;
			const float rectHeightFactor = 1.1f; // Make the new rect a bit taller than this glyph
			int rectHeight = (int)roundf(float(glyphHeight) * rectHeightFactor);
			rect.bottom = top + rectHeight;
			bestRect = rect;
		}

		uint32_t newWidth = page.width;
		uint32_t newHeight = page.height;

		// Check if glyph can fit on the right side of this rectangle
		IntRect extended = bestRect;
		extended.right += glyphWidth;
		while ((int64_t)extended.right > (int64_t)newWidth || (int64_t)extended.bottom > (int64_t)newHeight)
		{
			// Glyph doesn't fit. Expand texture size until it fits or reaches max texture size.
			uint32_t powerOf2 = 1;
			while (true)
			{
				if (powerOf2 > context->GetMaxTextureSize()) // Reached max texture size
				{
					Log(LogType::Warning, ToString("Failed to load new glyph for font '", fontDesc.fontName,
						"'. Reason: Reached max texture size (", context->GetMaxTextureSize(), ")."));
					return IntRect(0, 0, 0, 0);
				}
				if (powerOf2 > newWidth) break;
				if (powerOf2 > newHeight) break;
				powerOf2 *= 2;
			}
			if (newWidth < powerOf2) newWidth = powerOf2;
			if (newHeight < powerOf2) newHeight = powerOf2;
		}

		// If texture changed size, update pixel buffer
		if (newWidth != page.width || newHeight != page.height)
		{
			std::vector<byte> newPixels;
			newPixels.resize((size_t)newWidth * newHeight, 0);
			for (uint32_t y = 0; y < page.height; y++)
			{
				for (uint32_t x = 0; x < page.width; x++)
				{
					newPixels[((size_t)y * newWidth) + x] = page.pixels[((size_t)y * page.width) + x];
				}
			}
			page.pixels.swap(newPixels);
			page.width = newWidth;
			page.height = newHeight;
			page.dirty = true;
		}

		// Glyph fits.
		if (foundMatch)
		{
			page.rects[bestIndex] = extended; // Extended existing rect
		}
		else
		{
			page.rects.push_back(extended); // Extended a new rect
		}
		IntRect room = extended;
		room.left = extended.right - glyphWidth;
		return room;
	}

	void Font::PlaceGlyph(const byte* bitmap, uint16_t texPosX, uint16_t texPosY, uint16_t bitmapWidth, uint16_t bitmapHeight)
	{
		if (!isLoaded) return;
		if (isPrebaked) return;
		if (bitmapWidth == 0 || bitmapHeight == 0 || !bitmap) return;

		// Write pixels
		for (int y = 0; y < bitmapHeight; y++)
		{
			int pixelY = texPosY + y;
			for (int x = 0; x < bitmapWidth; x++)
			{
				int pixelX = texPosX + x;
				int p = pixelX + (page.width * pixelY);
				byte alpha = bitmap[x + (bitmapWidth * y)];
				//page.pixels[p] = (0x01000000 * alpha) + 0xFFFFFF; // 32 bit format
				page.pixels[p] = alpha; // 8 bit format
			}
		}

		page.dirty = true;
	}


} //end of namespace ig