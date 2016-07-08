# (!) UNDER CONSTRUCTION

# CUMES - C Unrestricted Mail Exchange Server

CUMES is (or will be) a free and secure MTA, partially inspired by qmail.

* Under construction
* Unrestricted: CUMES is not *Free, but with restrictions, Software*, but MIT-Licensed. You can do (almost) everything with the code.

## Motivation

Every few months, or even days, another security hole shows up in sendmail, postfix, exim and other mailers. There are many more holes waiting to be discovered. Sendmail for example had and has many security vulnerabilities, see [Sendmail Disaster](https://cr.yp.to/maildisasters/sendmail.html) and the actual table of [CVE-Details](https://www.cvedetails.com/vulnerability-list/vendor_id-31/Sendmail.html).

A list of my favorite CVEs so far:
* [CVE-2002-1337](https://www.cvedetails.com/cve/CVE-2002-1337/): A flaw in headers.c f#cks up sendmail
* [CVE-2003-0161](https://www.cvedetails.com/cve/CVE-2003-0161/): A flaw in prescan() in parseaddr.c f#cks up sendmail
* [CVE-2003-0694](https://www.cvedetails.com/cve/CVE-2003-0694/): A flaw in prescan() in parseaddr.c f#cks up sendmail on more time
* [CVE-2010-4344](https://www.cvedetails.com/cve/CVE-2010-4344/): A flaw in string_vformat in string.c f#cks up exim


