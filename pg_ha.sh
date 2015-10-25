#master
mDATA=/home/$USER/pg92/data
port=5432
wal_level=hot_standby
max_wal_senders=2
wal_keep_segments=5
logging_collector=on
#standby
sDATA=/home/$USER/pg92/standby
mkdir $sDATA
chmod 700 $sDATA
sport=5433
shot_standby=on

pg_ctl stop -m i
pg_ctl stop -D $sDATA -m i
rm -rf $sDATA/*

#step
sed -i /port\ =/c\port\ =\ $port                                        $mDATA/postgresql.conf
sed -i /wal_level\ =/c\wal_level\ =\ $wal_level                         $mDATA/postgresql.conf
sed -i /max_wal_senders\ =/c\max_wal_senders\ =\ $max_wal_senders       $mDATA/postgresql.conf
sed -i /wal_keep_segments\ =/c\wal_keep_segments\ =\ $wal_keep_segments $mDATA/postgresql.conf
sed -i /logging_collector\ =/c\logging_collector\ =\ $logging_collector $mDATA/postgresql.conf
sed -i 's/#host/host/'  $mDATA/pg_hba.conf

#restart master
pg_ctl restart
psql -c "alter user $USER password '123456'"
psql -c "select pg_start_backup('label');"
#copy data
cp -r $mDATA/* $sDATA
psql -c "select pg_stop_backup();"

#modify standby port
sed -i /port\ =/c\port\ =\ $sport                       $sDATA/postgresql.conf
sed -i /hot_standby\ =/c\hot_standby\ =\ $shot_standby   $sDATA/postgresql.conf
rm -rf $sDATA/postmaster.pid
#recovery.conf
cp $PGHOME/share/recovery.conf.sample  $sDATA/recovery.conf
sed -i /standby_mode/c\standby_mode=on $sDATA/recovery.conf
sed -i /primary_conninfo\ =/c\primary_conninfo="'host=127.0.0.1 port=5432 user=$USER password=123456'"   $sDATA/recovery.conf
sed -i /trigger_file/c\trigger_file="'$sDATA/trigger'" $sDATA/recovery.conf


pg_ctl start -D $sDATA
