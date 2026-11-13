<?php
use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;
return new class extends Migration {
 public function up():void {
    Schema::create('regions',function(Blueprint $t){$t->id();$t->string('name',50);$t->string('slug',20)->unique();$t->timestamps();});
    Schema::create('providers',function(Blueprint $t){$t->id();$t->string('name',180);$t->string('city',120);$t->string('postal_code',10)->nullable();$t->string('phone',50)->nullable();$t->string('status',30)->default('unavailable');$t->unsignedTinyInteger('capacity')->default(0);$t->timestamp('last_seen_at')->nullable();$t->timestamps();});
    Schema::create('provider_region',function(Blueprint $t){$t->foreignId('provider_id')->constrained()->cascadeOnDelete();$t->foreignId('region_id')->constrained()->cascadeOnDelete();$t->primary(['provider_id','region_id']);});
    Schema::create('devices',function(Blueprint $t){$t->id();$t->foreignId('provider_id')->constrained()->cascadeOnDelete();$t->string('device_uid',80)->unique();$t->string('token_hash',64);$t->timestamp('last_seen_at')->nullable();$t->timestamps();});
    Schema::create('status_histories',function(Blueprint $t){$t->id();$t->foreignId('provider_id')->constrained()->cascadeOnDelete();$t->foreignId('device_id')->nullable()->constrained()->nullOnDelete();$t->string('status',30);$t->unsignedTinyInteger('capacity');$t->timestamp('created_at')->useCurrent();$t->index(['provider_id','created_at']);});
 }
 public function down():void {Schema::dropIfExists('status_histories');Schema::dropIfExists('devices');Schema::dropIfExists('provider_region');Schema::dropIfExists('providers');Schema::dropIfExists('regions');}
};
