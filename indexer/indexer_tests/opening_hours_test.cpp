#include "../../testing/testing.hpp"

#include "../../3party/opening_hours/osm_time_range.hpp"

UNIT_TEST(OpeningHours_Parse)
{
  {
    OSMTimeRange oh = OSMTimeRange::FromString("06:00-09:00/03");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("06:00-07:00/03");
    TEST(oh.IsValid() == false, ());
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("sunrise-sunset");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Su-Th sunset-24:00, 04:00-sunrise; Fr-Sa sunset-sunrise");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Apr-Sep Su [1,3] 14:30-17:00");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("06:00+");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("06:00-07:00+");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("24/7");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("06:13-15:00");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Mo-Su 08:00-23:00");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("(sunrise+02:00)-(sunset-04:12)");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Mo-Sa; PH off");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Jan-Mar 07:00-19:00;Apr-Sep 07:00-22:00;Oct-Dec 07:00-19:00");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Mo closed");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("06:00-23:00 open \"Dining in\"");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("06:00-23:00 open \"Dining in\" || 00:00-24:00 open \"Drive-through\"");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Tu-Th 20:00-03:00 open \"Club and bar\"; Fr-Sa 20:00-04:00 open \"Club and bar\" || Su-Mo 18:00-02:00 open \"bar\" || Tu-Th 18:00-03:00 open \"bar\" || Fr-Sa 18:00-04:00 open \"bar\"");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("09:00-21:00 \"call us\"");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("10:00-13:30,17:00-20:30");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Apr-Sep: Mo-Fr 09:00-13:00,14:00-18:00; Apr-Sep: Sa 10:00-13:00");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Mo,We,Th,Fr 12:00-18:00; Sa-Su 12:00-17:00");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Su-Th 11:00-03:00, Fr-Sa 11:00-05:00");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Mo-We 17:00-01:00, Th,Fr 15:00-01:00; PH off");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    /* test disabled because we go out from DSL definition in some cases */
    //    OSMTimeRange oh = OSMTimeRange::FromString("Tu-Su, Ph 10:00-18:00");
    //    TEST(oh.IsValid() == false, ("Broken parser"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Tu-Su 10:00-18:00, Mo 12:00-17:00");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("06:00-07:00/21:03");
    TEST(oh.IsValid() == false, ("Period can't be large then interval"));
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("sunset-sunrise");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
  }
}

UNIT_TEST(OpeningHours_TimeHit)
{
  {
    OSMTimeRange oh = OSMTimeRange::FromString("06:13-15:00; 16:30+");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
    TEST(oh("12-12-2013 7:00").IsOpen(), ());
    TEST(oh("12-12-2013 16:00").IsClosed(), ());
    TEST(oh("12-12-2013 20:00").IsOpen(), ());
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("We-Sa; Mo[1,3] closed; Su[-1,-2] closed; Fr[2] open; Fr[-2], Fr open; Su[-2] -2 days");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
    TEST(oh("20-03-2015 18:00").IsOpen(), ());
    TEST(oh("17-03-2015 18:00").IsClosed(), ());
  }

  {
    OSMTimeRange oh = OSMTimeRange::FromString("We-Fr; Mo[1,3] closed; Su[-1,-2] closed");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
    TEST(oh("20-03-2015 18:00").IsOpen(), ());
    TEST(oh("17-03-2015 18:00").IsClosed(), ());
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("We-Fr; Mo[1,3] +1 day closed; Su[-1,-2] -3 days closed");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
    TEST(oh("20-03-2015 18:00").IsOpen(), ());
    TEST(oh("17-03-2015 18:00").IsClosed(), ());
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Mo-Su 14:30-17:00; Mo[1] closed; Su[-1] closed");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
    TEST(oh("09-03-2015 16:00").IsOpen(), ());
    TEST(oh("02-03-2015 16:00").IsClosed(), ());
    TEST(oh("22-03-2015 16:00").IsOpen(), ());
    TEST(oh("29-03-2015 16:00").IsClosed(), ());
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("PH,Tu-Su 10:00-18:00; Sa[1] 10:00-18:00 open \"Eintritt ins gesamte Haus frei\"; Jan 1,Dec 24,Dec 25,easter -2 days: closed");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
    TEST(oh("03-03-2015 16:00").IsOpen(), ());
    TEST(oh.Comment().empty(), ());
    TEST(oh("07-03-2015 16:00").IsOpen(), ());
    TEST(oh.Comment().empty() == false, ());
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("Mo-Su 11:00+; Mo [1,3] off");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
    TEST(oh("04-03-2015 16:00").IsOpen(), ());
    TEST(oh("09-03-2015 16:00").IsOpen(), ());
    TEST(oh("02-03-2015 16:00").IsClosed(), ());
    TEST(oh("16-03-2015 16:00").IsClosed(), ());
  }
  {
    OSMTimeRange oh = OSMTimeRange::FromString("08:00-16:00 open, 16:00-03:00 open \"public room\"");
    TEST(oh.IsValid(), ("Incorrect schedule string"));
    TEST(oh("01-03-2015 20:00").IsOpen(), ());
    TEST(oh("01-03-2015 20:00").Comment() == "public room", ());
  }
}
