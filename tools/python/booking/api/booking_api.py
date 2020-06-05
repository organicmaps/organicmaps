import json
import logging
from functools import partial
from random import randint
from threading import Event
from time import sleep

import requests
from ratelimit import limits, sleep_and_retry

from .exceptions import AttemptsSpentError, HTTPError

LIMIT_REQUESTS_PER_MINUTE = 400
ATTEMPTS_COUNT = 10
MINMAX_LIMIT_WAIT_AFTER_429_ERROR_SECONDS = (30, 120)


class BookingApi:
    ENDPOINTS = {"countries": "list", "hotels": "list", "districts": "list"}

    def __init__(self, login, password, version):
        major_minor = version.split(".")
        assert len(major_minor) == 2
        assert int(major_minor[0]) >= 2
        assert 0 <= int(major_minor[1]) <= 4

        self._event = Event()
        self._event.set()
        self._timeout = 5 * 60  # in seconds
        self._login = login
        self._password = password
        self._base_url = f"https://distribution-xml.booking.com/{version}/json"
        self._set_endpoints()

    @sleep_and_retry
    @limits(calls=LIMIT_REQUESTS_PER_MINUTE, period=60)
    def call_endpoint(self, endpoint, **params):
        self._event.wait()
        try:
            attempts = ATTEMPTS_COUNT
            while attempts:
                attempts -= 1
                response = None
                try:
                    response = requests.get(
                        f"{self._base_url}/{endpoint}",
                        auth=(self._login, self._password),
                        params=params,
                        timeout=self._timeout,
                    )
                except requests.exceptions.ReadTimeout:
                    logging.exception("Timeout error.")
                    continue
                except requests.exceptions.SSLError:
                    logging.exception("SSL error.")
                    continue

                code = response.status_code
                try:
                    data = response.json()
                except json.decoder.JSONDecodeError:
                    logging.exception(
                        f"JSON decode error. " f"Content: {response.content}"
                    )
                    continue

                if code == 200:
                    return data["result"]
                else:
                    self._handle_http_errors(code, data)
            raise AttemptsSpentError(f"{ATTEMPTS_COUNT} attempts were spent.")
        except Exception as e:
            if not self._event.is_set():
                self._event.set()
            raise e

    def _handle_http_errors(self, code, data):
        if code == 429:
            self._event.clear()
            wait_seconds = randint(*MINMAX_LIMIT_WAIT_AFTER_429_ERROR_SECONDS)
            logging.warning(
                f"Http error {code}: {data}. "
                f"It waits {wait_seconds} seconds and tries again."
            )
            sleep(wait_seconds)
            self._event.set()
        else:
            raise HTTPError(f"Http error with code {code}: {data}.")

    def _set_endpoints(self):
        for endpoint in BookingApi.ENDPOINTS:
            setattr(self, endpoint, partial(self.call_endpoint, endpoint))


class BookingListApi:
    _ROWS_BY_REQUEST = 1000

    def __init__(self, api):
        self.api = api
        self._set_endpoints()

    def call_endpoint(self, endpoint, **params):
        result = []
        offset = 0
        while True:
            resp = self._call_endpoint_offset(offset, endpoint, **params)
            result.extend(resp)
            if len(resp) < BookingListApi._ROWS_BY_REQUEST:
                break
            offset += BookingListApi._ROWS_BY_REQUEST
        return result

    def _call_endpoint_offset(self, offset, endpoint, **params):
        r = self.api.call_endpoint(
            endpoint,
            **{"offset": offset, "rows": BookingListApi._ROWS_BY_REQUEST, **params},
        )
        if not isinstance(r, list):
            raise TypeError(f"Result has unexpected type {type(r)}")
        return r

    def _set_endpoints(self):
        for endpoint in BookingApi.ENDPOINTS:
            if BookingApi.ENDPOINTS[endpoint] == "list":
                setattr(self, endpoint, partial(self.call_endpoint, endpoint))
