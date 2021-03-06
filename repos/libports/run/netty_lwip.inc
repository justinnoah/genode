create_boot_directory
import_from_depot [depot_user]/src/[base_src] \
                  [depot_user]/pkg/[drivers_nic_pkg] \
                  [depot_user]/src/init \
                  [depot_user]/src/libc \
                  [depot_user]/src/vfs_audit \
                  [depot_user]/src/vfs_lwip \
                  [depot_user]/src/vfs

append config {
<config verbose="yes">
	<parent-provides>
		<service name="ROM"/>
		<service name="IRQ"/>
		<service name="IO_MEM"/>
		<service name="IO_PORT"/>
		<service name="PD"/>
		<service name="RM"/>
		<service name="CPU"/>
		<service name="LOG"/>
		<service name="SIGNAL"/>
	</parent-provides>
	<default-route>
		<any-service> <parent/> <any-child/> </any-service>
	</default-route>
	<default caps="100"/>
	<start name="timer">
		<resource name="RAM" quantum="1M"/>
		<provides> <service name="Timer"/> </provides>
	</start>
	<start name="drivers" caps="1000" managing_system="yes">
		<resource name="RAM" quantum="32M"/>
		<binary name="init"/>
		<route>
			<service name="ROM" label="config"> <parent label="drivers.config"/> </service>
			<service name="Timer"> <child name="timer"/> </service>
			<any-service> <parent/> </any-service>
		</route>
		<provides> <service name="Nic"/> </provides>
	</start>
	<start name="socket_fs" caps="200">
		<binary name="vfs"/>
		<resource name="RAM" quantum="32M"/>
		<provides> <service name="File_system"/> </provides>
		<route>
			<service name="ROM" label="socket_fs.config"> <child name="dynamic_rom"/> </service>
			<any-service> <parent/> <any-child/> </any-service>
		</route>
		<config ld_verbose="yes">
			<vfs>
				<dir name="lwip">
					<lwip ip_addr="10.0.2.55" netmask="255.255.255.0" gateway="10.0.2.1" nameserver="8.8.8.8"/>
					<!-- <lwip dhcp="yes"/> -->
				</dir>
				<dir name="socket">
					<audit path="/lwip"/>
				</dir>
			</vfs>
			<default-policy root="/socket" writeable="yes" />
		</config>
	</start>
}

append qemu_args " -nographic "
append_qemu_nic_args

#proc qemu_nic_model {} {
#       if [have_spec x86]        { return e1000 }
#       if [have_board pbxa9]     { return lan9118 }
#       if [have_board zynq_qemu] { return cadence_gem }
#       return nic_model_missing
#}
#append qemu_args " -net nic,model=[qemu_nic_model] "
#append qemu_args " -net tap,ifname=tap0,downscript=no,script=no "

# vi: set ft=tcl :
