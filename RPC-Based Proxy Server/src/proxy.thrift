/* Simple proxy server 
 * argument to fetch_page call is the URL
*/

namespace cpp proxy

service proxy {
	string fetch_page(1:string url);
}
