В файле osm-map-features-z12.xml есть косяки с пропущенными тире в описании стилей.
Мержить его ТОЛЬКО вручную (оставлять тире в нашей версии).
Мерж удобно делать при помощи git diff data/<filename>

После перегенерации classificator.txt надо:
 - Удалить тэги первого уровня:
	access, cycleway, junction
 - Замержить place: country, city, town, county, continent

 - Убрать стили отрисовки имен улиц для 14-го масштаба (14|5|x) у мелких дорог (которые не рисуются в 12 масштабе):
	unclassified, residential, living_street, tertiary, tertiary_link
	
 - Запустить прогу и сохранить visibility.txt, замержить:
	- забрать все настройки невидимости;
	- забрать настройки видимости по:
		- boundary: administrative:2
		- amenity: fuel, amenity:restaurant;
		- place: ...
 