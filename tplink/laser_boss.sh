#!/bin/sh

API_URL=https://ssl.acemonstertoys.org/laser/api.php
API_KEY=mYsEcReTkEy

ODOMETER_PORT=/dev/ttyACM0
#ODOMETER_PORT=/dev/tty

stty -F ${ODOMETER_PORT} raw speed 9600

MSG_DELAY=4

ENABLED=false
CURRENT_RFID=
CURRENT_ODOMETER=

make_hash()
{
  HASH=`echo -n $1${API_KEY} | sha256sum`
  echo ${HASH%% -}
}

query_api()
{
  TS=`date +%s`
  QUERYSTR=$1"&ts="${TS}
  QUERYSTR=${QUERYSTR}"&hash="`make_hash ${QUERYSTR}`
  wget -q ${API_URL}"?"${QUERYSTR} -O - --no-check-certificate
}

# $1 = ID $2 = odometer
laser_login()
{
  QUERY="cmd=login&rfid=$1&odometer=$2"
  query_api ${QUERY}
}

# $1 = ID $2 = odometer
laser_logout()
{
  QUERY="cmd=logout&odometer=$2"
  query_api ${QUERY}
}

enable_laser() {
  #echo "ENABLE"
  echo "e" > ${ODOMETER_PORT}
}

enable_laser_until() {
  echo "u"$1 > ${ODOMETER_PORT}
}

disable_laser() {
  #echo "DISABLE"
  echo "d" > ${ODOMETER_PORT}
}

display() {
  #echo "Displaying " $1
  echo "p"$1 > ${ODOMETER_PORT}
}

disable_laser

if [ ! -f /var/state/ntp_ok ]; then
  display "   Waiting for NTP"
  ntpd -n -q -p 0.openwrt.pool.ntp.org -p 1.openwrt.pool.ntp.org -p 2.openwrt.pool.ntp.org -p 3.openwrt.pool.ntp.org
  if [ $? == 0 ]; then
    touch /var/state/ntp_ok
  fi
fi

while [ ! -f /var/state/ntp_ok ]; do
  sleep 1
done

display "   Present Key"

DATA=.
while [ "$DATA" != "exit" ]
do
  read DATA < ${ODOMETER_PORT}

  if [ "$DATA" != "" ]; then
    if [ "${DATA:0:1}" == "o" ]; then
      CURRENT_ODOMETER=${DATA:1}
      echo ${CURRENT_ODOMETER}
    elif [ "${DATA:0:1}" == "r" ]; then
      DATA=${DATA:1}
      CURRENT_ODOMETER=${DATA%%,*}
      NEW_RFID=${DATA##*,}

      if [ "${ENABLED}" == "true" ]; then
        disable_laser
        display "      Wait"
        RESPONSE=`laser_logout ${NEW_RFID} ${CURRENT_ODOMETER}`
        #echo "Logout response: " ${RESPONSE}
        ENABLED=false
        CURRENT_RFID=
        display "  Logged Out"
        sleep ${MSG_DELAY}
        display "   Present Key"
      else
        display "      Wait"
        RESPONSE=`laser_login ${NEW_RFID} ${CURRENT_ODOMETER}`
        #echo "Login response: " ${RESPONSE}
        if [ "${RESPONSE%%|*}" == "true" ]; then
          CURRENT_RFID=${NEW_RFID}
          ENABLED=true
          USER_NAME=${RESPONSE##*|}
          enable_laser
          display ${USER_NAME}
          sleep ${MSG_DELAY}
          display "   Logged In"
          sleep ${MSG_DELAY} 
          display "Laser Odometer"
        else
          disable_laser
          ENABLED=false
          CURRENT_RFID=
          display "Login Failure"
          sleep ${MSG_DELAY} 
          if [ "${RESPONSE}" == "" ]; then
            display "Network Error"
          else
            display "${RESPONSE##*|}"
          fi
          sleep ${MSG_DELAY} 
          display "   Present Key"
        fi
      fi
    fi
  fi
done
