create table request_log (
	id serial primary key,
	server_timestamp timestamp not null,
	child_timestamp timestamp not null,
	url text not null,
	child_uuid text not null,
	response_code integer not null,
	category_group_id integer
);

create index request_log_server_timestamp_idx on request_log (server_timestamp);
create index request_log_child_uuid_idx on request_log (child_uuid);
