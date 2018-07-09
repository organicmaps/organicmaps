from __future__ import print_function

import jsons
import logging
import os


BIG_FILE_SIZE = 47684


class Payload:
    def __init__(self, message, response_code=200, headers={}):
        self.__response_code = response_code
        self.__message = message
        self.__headers = headers
        
    
    def response_code(self):
        """
        Response code to send to the client.
        """
        return self.__response_code
    
    
    def message(self):
        """
        The message to send to the client.
        """
        return self.__message


    def length(self):
        """
        The length of the response.
        """
        return len(self.message())


    def headers(self):
        """
        The headers to be sent to the client. Please, note, that these do not include
        the Content-Length header, which you need to send separately.
        """
        return self.__headers


    def __repr__(self):
        return "{}: {}: {}".format(self.response_code(), self.length(), self.message())


class ResponseProviderMixin:
    """
A mixin (basically, an interface) that the web-server that we might use relies on. 

In this implementation, the job of the web-server is just to get the request
(the url and the headers), and to send the response as it knows how. It isn't
its job to decide how to respond to what request. It is the job of the 
ResponseProvider.

In your web-server you should initialize the ResponseProvider, and ask it for

response_for_url_and_headers(url, headers)

Which will return a Payload object that the server must send as response.

The server might be notified when a particular request has been received:

got_pinged(self) - someone sent a ping request. The Response provider will
respond with "pong" and call this method of the server. You might want to
increment the count of active users, for ping is the request that new instances
of servers send to check if other servers are currently serving.

kill(self) - someone sent the kill request, which means that that someone
no longer needs this server to serve. You might want to decrement the count of
active users and/or stop the server. 
"""
    
    def dispatch_response(self, payload):
        """
    Define this mehtod to dispatch the response received from the ResponseProvider
    """
        raise NotImplementedError()
    

    def got_pinged(self):
        """
    A ping request has been received. In most scenarios it means that the number of 
    users of this server has increased by 1.
    """
        raise NotImplementedError()
    
        
    def kill(self):
        """
    Someone no longer needs this server. Decrement the number of users and stop
    the server if the number fell to 0.
    """
        raise NotImplementedError()


class ResponseProvider:
    def __init__(self, delegate):
        self.headers = list()
        self.delegate = delegate
        self.byterange = None
        self.is_chunked = False
        self.response_code = 200
   
    
    def pong(self):
        self.delegate.got_pinged()
        return Payload("pong")


    def my_id(self):
        return Payload(str(os.getpid()))


    def strip_query(self, url):
        query_start = url.find("?")
        if (query_start > 0):
            return url[:query_start]
        return url


    def response_for_url_and_headers(self, url, headers):
        self.headers = headers
        self.chunk_requested()
        url = self.strip_query(url)
        try:
            return {
                "/unit_tests/1.txt": self.test1,
                "/unit_tests/notexisting_unittest": self.test_404,
                "/unit_tests/permanent": self.test_301,
                "/unit_tests/47kb.file": self.test_47_kb,
                # Following two URIs are used to test downloading failures on different platforms.
                "/unit_tests/mac/1234/Uruguay.mwm": self.test_404,
                "/unit_tests/linux/1234/Uruguay.mwm": self.test_404,
                "/ping": self.pong,
                "/kill": self.kill,
                "/id": self.my_id,
                "/partners/time": self.partners_time,
                "/partners/price": self.partners_price,
                "/booking/hotelAvailability": self.partners_hotel_availability,
                "/booking/deals": self.partners_hotels_with_deals,
                "/booking/blockAvailability": self.partners_block_availability,
                "/partners/taxi_info": self.partners_yandex_taxi_info,
                "/partners/get-offers-in-bbox/": self.partners_rent_nearby,
                "/partners/CalculateByCoords": self.partners_calculate_by_coords,
            }[url]()
        except:
            return self.test_404()


    def chunk_requested(self):
        if "range" in self.headers:
            self.is_chunked = True
            self.response_code = 206
            meaningful_string = self.headers["range"][6:]
            first, last = meaningful_string.split("-")
            self.byterange = (int(first), int(last))


    def trim_message(self, message):
        if not self.is_chunked:
            return message
        return message[self.byterange[0]: self.byterange[1] + 1]


    def test1(self):
        init_message = "Test1"
        message = self.trim_message(init_message)
        size = len(init_message)
        self.check_byterange(size)
        headers = self.chunked_response_header(size)
        
        return Payload(message, self.response_code, headers)


    def test_404(self):
        return Payload("", response_code=404)


    def test_301(self):
        return Payload("", 301, {"Location" : "google.com"})
    

    def check_byterange(self, size):
        if self.byterange is None:
            self.byterange = (0, size)

    def chunked_response_header(self, size):
        return {
            "Content-Range" : "bytes {start}-{end}/{out_of}".format(start=self.byterange[0],
            end=self.byterange[1], out_of=size)
        }
        
    
    def test_47_kb(self):
        self.check_byterange(BIG_FILE_SIZE)
        headers = self.chunked_response_header(BIG_FILE_SIZE)
        message = self.trim_message(self.message_for_47kb_file())
            
        return Payload(message, self.response_code, headers)

    
    def message_for_47kb_file(self):
        message = []
        for i in range(0, BIG_FILE_SIZE + 1):
            message.append(chr(i / 256))
            message.append(chr(i % 256))

        return "".join(message)


    # Partners_api_tests
    def partners_time(self):
        return Payload(jsons.PARTNERS_TIME)


    def partners_price(self):
        return Payload(jsons.PARTNERS_PRICE)

    def partners_hotel_availability(self):
        return Payload(jsons.HOTEL_AVAILABILITY)

    def partners_hotels_with_deals(self):
        return Payload(jsons.HOTELS_WITH_DEALS)

    def partners_block_availability(self):
        return Payload(jsons.BLOCK_AVAILABILITY)

    def partners_yandex_taxi_info(self):
        return Payload(jsons.PARTNERS_TAXI_INFO)

    def partners_rent_nearby(self):
        return Payload(jsons.PARTNERS_RENT_NEARBY)

    def partners_calculate_by_coords(self):
        return Payload(jsons.PARTNERS_CALCULATE_BY_COORDS)

    def kill(self):
        logging.debug("Kill called in ResponseProvider")
        self.delegate.kill()
        return Payload("Bye...")
