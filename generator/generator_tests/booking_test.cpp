#include "testing/testing.hpp"

#include "generator/booking_dataset.hpp"

UNIT_TEST(BookingDataset_SmokeTest)
{
  stringstream ss;
  generator::BookingDataset data(ss);
  TEST_EQUAL(data.Size(), 0, ());
}

UNIT_TEST(BookingDataset_ParseTest)
{
  stringstream ss(
      "1485988\t36.75818960879561\t3.053177244180233\tAppartement Alger Centre\t50 Avenue Ahmed "
      "Ghermoul\t0\t0\tNone\tNone\thttp://www.booking.com/hotel/dz/"
      "appartement-alger-centre-alger.html\t201\t\t\t");
  generator::BookingDataset data(ss);
  TEST_EQUAL(data.Size(), 1, ());
}

UNIT_TEST(BookingDataset_ParseTest2)
{
  stringstream ss(
      "1485988\t36.75818960879561\t3.053177244180233\tAppartement Alger Centre\t50 Avenue Ahmed "
      "Ghermoul\t0\t0\tNone\tNone\thttp://www.booking.com/hotel/dz/"
      "appartement-alger-centre-alger.html\t201\t\t\t\n"
      "357811\t34.86737239675703\t-1.31686270236969\tRenaissance Tlemcen Hotel\tRenaissance "
      "Tlemcen "
      "Hotel\t5\t2\tNone\tNone\thttp://www.booking.com/hotel/dz/"
      "renaissance-tlemcen.html\t204\t\t\t\n"
      "1500820\t36.72847621708523\t3.0645270245369147\tMazghana Apartment\tCite Garidi 1 Tours 3 N "
      "53, "
      "Kouba\t0\t0\tNone\tNone\thttp://www.booking.com/hotel/dz/"
      "mazghana-apartment.html\t201\t\t\t\n"
      "1318991\t35.692865978372666\t-0.6278949570083796\tBest Western Hotel Colombe\t6 Bd Zabour "
      "Larbi  Hai Khaldia "
      "Delmonte\t4\t2\tNone\tNone\thttp://www.booking.com/hotel/dz/"
      "best-western-colombe.html\t204\t\t\t\n"
      "1495828\t36.33835943\t6.626214981\tConstantine Marriott Hotel\tOued Rhumel Street, Cites "
      "des Arcades "
      "Romaines,\t5\t2\tNone\tNone\thttp://www.booking.com/hotel/dz/"
      "constantine-marriott.html\t204\t\t\t\n"
      "1411999\t35.73994643933386\t-0.757756233215332\tResidence Nadra\tBoulevard de la plage, "
      "Niche 1236 Paradis "
      "plage\t0\t1\tNone\tNone\thttp://www.booking.com/hotel/dz/residence-nadra.html\t201\t\t\t\n"
      "1497769\t36.80667121575615\t3.231203541069817\tApartment La Pérouse\tLa Pérouse Ain "
      "Taya\t0\t0\tNone\tNone\thttp://www.booking.com/hotel/dz/"
      "apartment-la-perouse.html\t220\t\t\t\n"
      "1668244\t36.715150622433804\t2.8442734479904175\tAZ Hotel Zeralda\t09 Rue de Mahelma - "
      "Zeralda - "
      "ALGER\t4\t2\tNone\tNone\thttp://www.booking.com/hotel/dz/el-aziz-zeralda.html\t204\t\t\t\n"
      "1486823\t36.73432645678891\t3.0335435271263123\tGuest House Marhaba\tResidence Soumam - "
      "Bloc B - Appt 17- Said "
      "Hamdine\t0\t0\tNone\tNone\thttp://www.booking.com/hotel/dz/marhaba.html\t208\t\t\t\n"
      "1759799\t35.73832476589291\t-0.7553583383560181\tHotel la brise\tAngle boulevard de la "
      "plage et route nationale niche 1159 paradis "
      "plage\t2\t2\tNone\tNone\thttp://www.booking.com/hotel/dz/la-brise.html\t204\t\t\t\n");
  generator::BookingDataset data(ss);
  TEST_EQUAL(data.Size(), 10, ());
}
