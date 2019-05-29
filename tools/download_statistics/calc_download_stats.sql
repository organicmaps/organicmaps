drop table if exists downloads;
create table downloads(year int, month int, country_from varchar(255), country_to varchar(255), type char(1), count int);

load data infile '/tmp/aggregated_download_info.csv' into table downloads fields terminated by ';' lines terminated by '\n';

delete from downloads where country_to in (select country_to from (select country_to, SUM(count) as cnt from downloads group by country_to having cnt < 1000) as z);

delete from downloads where country_from in ('None', 'Unknown');

update downloads set country_from='USA'                          where country_from='United States';
update downloads set country_from='UK'                           where country_from='United Kingdom';
update downloads set country_from='Moldova'                      where country_from='Republic of Moldova';
update downloads set country_from='Lithuania'                    where country_from='Republic of Lithuania';
update downloads set country_from='Burma'                        where country_from='Myanmar [Burma]';
update downloads set country_from='China'                        where country_from='Hong Kong';
update downloads set country_from='China'                        where country_from='Macao';
update downloads set country_from='South Korea'                  where country_from='Republic of Korea';
update downloads set country_from='Jordan'                       where country_from='Hashemite Kingdom of Jordan';
update downloads set country_from='Italy'                        where country_from='San Marino';
update downloads set country_from='Micronesia'                   where country_from='Federated States of Micronesia';
update downloads set country_from='United States Virgin Islands' where country_from='U.S. Virgin Islands';
update downloads set country_from='Congo-Kinshasa'               where country_from='Congo';
update downloads set country_from='Congo-Brazzaville'            where country_from='Republic of the Congo';
update downloads set country_from='Saint Martin'                 where country_from='Sint Maarten';
update downloads set country_from='Netherlands Antilles'         where country_from='Bonaire';

select distinct country_from as country_from_that_never_to from downloads where country_from not in (select country_to from downloads);
select distinct country_to as country_to_that_never_from from downloads where country_to not in (select country_from from downloads);

select country_to as most_downloaded_country, SUM(count) as cnt from downloads where type='M' group by country_to order by cnt desc limit 20;
select country_to as most_downloaded_country_from_outside, SUM(count) as cnt from downloads where type='M' and country_from <> country_to group by country_to order by cnt desc limit 20;
select country_from as most_downloading_country, SUM(count) as cnt from downloads where type='M' group by country_from order by cnt desc limit 20;


select country_from, in_cnt, out_cnt, out_cnt * 1.0 / in_cnt as ratio from \
  (select country_from, SUM(in_cnt) as in_cnt, SUM(out_cnt) as out_cnt from ( \
          select country_from, SUM(count) as in_cnt, 0 as out_cnt from downloads where type='M' and country_from=country_to group by country_from \
    union select country_from, 0 as in_cnt, SUM(count) as out_cnt from downloads where type='M' and country_from<>country_to group by country_from \
  ) as z group by country_from \
) as zz order by ratio desc;
