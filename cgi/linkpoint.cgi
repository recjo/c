#include <string.h>
#include <stdio.h>
#include "ccapi_client.h"
#include "ccapi_error.h"
#include "cgi-lib.h"
#include "string-lib.h"
#include <signal.h>

// routines 
int  checkVals(void);
int  checkExistence(void);
void getCGIvars(void);
void doLinkPoint(void);

// LinkPoint Vars
float total;
int reqtype, port, result;
char buf[1024];
int i;
OrderCtx *order;
ReqCtx *req;

// variables for CGI form data 
llist entries;
int  status;
int  snoopdog = 1;
int  busby = 1; 
char *first;
char *last;
char fullname[150];
char *astcard;
char *email;
char *address;
char *city;
char *state;
char *zip;
char *country;
char *saddress;
char *scity;
char *sstate;
char *szip;
char *scountry;
char *hphone;
char *wphone;
char *cardtype;
char *cardnumber;
char *cardname;
char *expmo;
char *expyr;
char *chgtotal;
char *username;
char *avsstr;
char *errStr = "Not all information was filled in. Please <a href=\"javascript:hist
ory.back();\">go back</a> and fill in all required fields.";
char *errStr2 = "Looks like there are some variables not defined in the source html. 
Contact the site administrator.";

int main(int argc, char *argv[])
{
  status = read_cgi_input(&entries);
 
  if (checkExistence() && checkVals())
  {
    getCGIvars();
    doLinkPoint();
  }
  else
  {
    printf("Content-type: text/html\n\n");
    if (snoopdog == 0)
      printf("%s\n", errStr2);
    else if (busby == 0) 
      printf("%s\n", errStr);
  } 

  return (0);
}

int checkExistence(void)
{
  // check to see if source html defines correct variables (fields)
  if (! is_field_exists(entries, "first"))
    snoopdog = 0;
  if (! is_field_exists(entries, "last"))
    snoopdog = 0;
  if (! is_field_exists(entries, "email"))
    snoopdog = 0;
  if (! is_field_exists(entries, "address"))
    snoopdog = 0;
  if (! is_field_exists(entries, "city"))
    snoopdog = 0;
  if (! is_field_exists(entries, "state"))
    snoopdog = 0;
  if (! is_field_exists(entries, "zip"))
    snoopdog = 0;
  if (! is_field_exists(entries, "country"))
    snoopdog = 0;
  if (! is_field_exists(entries, "shipping"))
    snoopdog = 0;
  if (! is_field_exists(entries, "city2"))
    snoopdog = 0;
  if (! is_field_exists(entries, "state2"))
    snoopdog = 0;
  if (! is_field_exists(entries, "zip2"))
    snoopdog = 0;
  if (! is_field_exists(entries, "country2"))
    snoopdog = 0;
  if (! is_field_exists(entries, "hphone"))
    snoopdog = 0;
  if (! is_field_exists(entries, "wphone"))
    snoopdog = 0;
  if (! is_field_exists(entries, "_username"))
    snoopdog = 0;
  if (! is_field_exists(entries, "cardtype"))
    snoopdog = 0;
  if (! is_field_exists(entries, "cardnumber"))
    snoopdog = 0;
  if (! is_field_exists(entries, "cardname"))
    snoopdog = 0;
  if (! is_field_exists(entries, "expmo"))
    snoopdog = 0;
  if (! is_field_exists(entries, "expyr"))
    snoopdog = 0;
  if (! is_field_exists(entries, "total"))
    snoopdog = 0;

  return (snoopdog);
}

int checkVals(void)
{ 
  // now see if any required variables are empty strings
  if (! strlen(cgi_val(entries,"first"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"last"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"email"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"address"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"city"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"state"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"zip"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"country"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"_username"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"cardtype"))>0)
    busby = 0;  
  if (! strlen(cgi_val(entries,"cardnumber"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"cardname"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"expmo"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"expyr"))>0)
    busby = 0;
  if (! strlen(cgi_val(entries,"total"))>0)
    busby = 0;

  return (busby); 
}

void getCGIvars(void)
{
  // put form data into variables
  first = newstr(cgi_val(entries,"first"));
  last = newstr(cgi_val(entries,"last"));
  strcat(fullname, first);
  strcat(fullname, " ");
  strcat(fullname, last);
  email = newstr(cgi_val(entries,"email"));
  address = newstr(cgi_val(entries,"address"));
  city = newstr(cgi_val(entries,"city"));
  state = newstr(cgi_val(entries,"state"));
  zip = newstr(cgi_val(entries,"zip"));
  country = newstr(cgi_val(entries,"country"));
  saddress = newstr(cgi_val(entries,"shipping"));
  scity = newstr(cgi_val(entries,"city2"));
  sstate = newstr(cgi_val(entries,"state2"));
  szip = newstr(cgi_val(entries,"zip2"));
  scountry = newstr(cgi_val(entries,"country2"));
  hphone = newstr(cgi_val(entries,"hphone"));
  wphone = newstr(cgi_val(entries,"wphone"));
  username = newstr(cgi_val(entries,"_username"));
  cardtype = newstr(cgi_val(entries,"cardtype"));
  cardnumber = newstr(cgi_val(entries,"cardnumber"));
  cardname = newstr(cgi_val(entries,"cardname"));
  expmo = newstr(cgi_val(entries,"expmo"));
  expyr = newstr(cgi_val(entries,"expyr"));
  chgtotal = newstr(cgi_val(entries,"total"));

  astcard = substr(cardnumber, strlen(cardnumber) - 4, 4);

  // get digits of address for AVS
  avsstr = strtok(address, " "); 
}

void doLinkPoint(void)
{
  printf("Name: %s <br>", fullname );
  printf("Card: %s <br>", cardtype );
  printf("Number: ************%s <br>", astcard );
  printf("Expires: %s/%s <p>", expmo, expyr );
  printf("Using ClearCommerce SSL API Version: %0.2f<br>\n", cc_util_version());
  
  cc_order_alloc(&order);
  cc_req_alloc(&req);

  port = 1139;
  cc_req_set(req, ReqField_Configfile, "370382");
  cc_req_set(req, ReqField_Keyfile, "/usr/local/stronghold_2.4.2/ssl/certs/datewatch.pem");
  cc_req_set(req, ReqField_Host, "secure.linkpt.net");
  cc_req_set(req, ReqField_Port, &port);

  if(cc_order_setrequest(order, req) != Succeed)
  {
    cc_util_errorstr(cc_order_error(order), buf, sizeof(buf));
    printf("%s<br>\n", buf);
  }

  /* Set order parameters */
  result = Result_Good;
  reqtype = Chargetype_Sale;

  cc_order_set(order, OrderField_Userid, username);
  cc_order_set(order, OrderField_Bname, fullname);
  cc_order_set(order, OrderField_Baddr1, address);
  cc_order_set(order, OrderField_Bcity, city);
  cc_order_set(order, OrderField_Bstate, state);
  cc_order_set(order, OrderField_Bzip, zip);
  cc_order_set(order, OrderField_Bcountry, country);
  cc_order_set(order, OrderField_Sname, fullname);
  cc_order_set(order, OrderField_Saddr1, saddress);
  cc_order_set(order, OrderField_Scity, scity);
  cc_order_set(order, OrderField_Sstate, sstate);
  cc_order_set(order, OrderField_Szip, szip);
  cc_order_set(order, OrderField_Scountry, scountry);
  cc_order_set(order, OrderField_Phone, hphone);
  cc_order_set(order, OrderField_Cardnumber, cardnumber);
  cc_order_set(order, OrderField_Chargetype, &reqtype);
  cc_order_set(order, OrderField_Expmonth, expmo); 
  cc_order_set(order, OrderField_Expyear, expyr);
  cc_order_set(order, OrderField_Email, email);
  cc_order_set(order, OrderField_Result, &result);
  cc_order_set(order, OrderField_Addrnum, avsstr);

  total = atof(chgtotal);
  cc_order_set(order, OrderField_Chargetotal, &total); 
  printf("Charge Total: %.2f<br>\n", total);

  if(cc_order_process(order) != Succeed)
  {
    cc_util_errorstr(cc_order_error(order), buf, sizeof(buf));
    printf("%s<br>\n", buf);
    printf("Location: http://datewatch.mrhm.com/join/disapproved.taf?username=%s\r\n\r\n",
 username); 
  }
  else
  {
    cc_order_get(order, OrderField_R_Time, buf, sizeof(buf));
    printf("Time - %s<br>\n", buf);
    
    cc_order_get(order, OrderField_R_Ref, buf, sizeof(buf));
    printf("Ref# - %s<br>\n", buf);
    
    cc_order_get(order, OrderField_R_Approved, buf, sizeof(buf));
    printf("Appr - %s<br>\n", buf);
    
    cc_order_get(order, OrderField_R_Code, buf, sizeof(buf));
    printf("Code - %s<br>\n", buf); 
    
    cc_order_get(order, OrderField_R_Error, buf, sizeof(buf));
    if (strlen(buf) > 0)
    {
      printf("Err  - %s<br>\n", buf);
    }
    cc_order_get(order, OrderField_R_Ordernum, buf, sizeof(buf));
    printf("Ord# - %s<br>\n", buf); 
  }
    
  cc_order_drop(order);
  cc_req_drop(req);  
  list_clear(&entries);
}
