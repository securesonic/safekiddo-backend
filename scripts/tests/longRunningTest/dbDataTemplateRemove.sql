-- profile categories groups list
delete from profile_categories_groups_list where code_profile_id=$profileId;

-- access control
delete from access_control where code_child_id=$childId;

-- device
delete from device where code_child_id=$childId;

-- child
delete from child where id=$childId;

-- profile
delete from profile where id=$profileId;

-- parent
delete from parent where id=$parentId;

